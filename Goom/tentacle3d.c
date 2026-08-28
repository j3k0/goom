#include <stdlib.h>

#include "v3d.h"
#include "surf3d.h"
#include "goom_tools.h"
#include "goom_config.h"
#include "goom_plugin_info.h"
#include "tentacle3d.h"

#define D 256.0f

#define nbgrid 6
#define definitionx 15
#define definitionz 45
/* scale applied to the transformation buffer's velocity when dragging the
 * tentacle skin with the image content (see "Flow Coupling" param) */
#define TENTACLE_FLOW_FORCE -3.f
/* Fade envelope (see "Fade" param): ramps the tentacle in/out smoothly at a
 * fixed slow rate. The "Fade" param controls only the sound-reactive
 * intensity, not the ramp speed. */
#define TENTACLE_FADE_STEP 0.008f  /* fade in/out speed per frame (~125 frames, ~2.5s) */
#define TENTACLE_FADE_SOUND_MAX 1.0f  /* max brightness boost from volume */
#define TENTACLE_GRID_MIN 3  /* min grid rows/columns; max = definitionx/definitionz */
/* tentacle modes: one is chosen each time the effect shows */
enum {
	TENTACLE_MODE_CLASSIC = 0,  /* random-waveform, timer-driven sweep (current) */
	TENTACLE_MODE_WAVE,         /* waveform curtain, beat-synced sweep, music-driven camera */
	TENTACLE_MODE_COUNT
};
#define TENTACLE_SPEED_THRESHOLD 0.12f /* sound.speedvar above this => fast-music mix */
#define TENTACLE_WAVE_PROB_FAST 50     /* percent of WAVE mode on fast music */
#define TENTACLE_WAVE_PROB_SLOW 20     /* percent of WAVE mode on slow music */
#define TENTACLE_CLASSIC_RAPPORT_MAX 1.12f /* classic intensity envelope cap */
#define TENTACLE_WAVE_RAPPORT_MAX 1.5f     /* wave-mode envelope cap (needs the gain below to bind) */
#define TENTACLE_WAVE_GAIN 1.55f           /* wave-only envelope multiplier before the clamp */
#define TENTACLE_WAVE_SURGE_MIN 0.6f       /* camera pull floor on a loud hit (wave) */
#define TENTACLE_WAVE_SURGE_MAX 1.4f       /* camera drop on quiet (wave) */
#define TENTACLE_WAVE_SWEEP_MIN 24         /* wave-mode beat-sweep duration (frames) */
#define TENTACLE_WAVE_SWEEP_RANGE 24
#define TENTACLE_WAVE_SMOOTH 0.4f          /* waveform-curtain temporal smoothing weight */

typedef struct _TENTACLE_FX_DATA {
	PluginParam enabled_bp;
	PluginParam flow_coupling_p;
	PluginParam fade_p;
	PluginParameters params;

	float cycle;
	grid3d *grille[nbgrid];
	float *vals;

#define NB_TENTACLE_COLORS 4
	int colors[NB_TENTACLE_COLORS];

	int col;
	int dstcol;
	float lig;
	float ligs;
	float fadeEnv;

	/* statics from pretty_move */
	float distt;
	float distt2;
	float rot; /* entre 0 et 2 * M_PI */
	int happens;
	int rotation;
	int lock;
	int prevDrawit;
	int mode;
} TentacleFXData;

static void tentacle_new (TentacleFXData *data);
static void tentacle_recreate (TentacleFXData *data);
static void tentacle_update(PluginInfo *goomInfo, Pixel *buf, Pixel *back, int W, int H,
                     short[2][512], float, int drawit, TentacleFXData *data);
static void tentacle_free (TentacleFXData *data);

/* 
 * VisualFX wrapper for the tentacles
 */

static void tentacle_fx_init(VisualFX *_this, PluginInfo *info) {
	
	TentacleFXData *data = (TentacleFXData*)malloc(sizeof(TentacleFXData));
	
	data->enabled_bp = secure_b_param("Enabled", 1);
	data->flow_coupling_p = secure_i_param ("Flow Coupling");
	IVAL(data->flow_coupling_p) = 75;
	IMIN(data->flow_coupling_p) = 0;
	IMAX(data->flow_coupling_p) = 100;
	ISTEP(data->flow_coupling_p) = 1;
	data->fade_p = secure_i_param ("Fade");
	IVAL(data->fade_p) = 0;
	IMIN(data->fade_p) = 0;
	IMAX(data->fade_p) = 100;
	ISTEP(data->fade_p) = 1;
	data->params = plugin_parameters ("3D Tentacles", 3);
	data->params.params[0] = &data->enabled_bp;
	data->params.params[1] = &data->flow_coupling_p;
	data->params.params[2] = &data->fade_p;

	data->cycle = 0.0f;
	data->col = (0x28<<(ROUGE*8))|(0x2c<<(VERT*8))|(0x5f<<(BLEU*8));
	data->dstcol = 0;
	data->lig = 1.15f;
	data->ligs = 0.1f;
	data->fadeEnv = 0.0f;
	
	data->distt = 10.0f;
	data->distt2 = 0.0f;
	data->rot = 0.0f; /* entre 0 et 2 * M_PI */
	data->happens = 0;
	
	data->rotation = 0;
	data->lock = 0;
	data->prevDrawit = 0;
	data->mode = TENTACLE_MODE_CLASSIC;
	data->colors[0] = (0x18<<(ROUGE*8))|(0x4c<<(VERT*8))|(0x2f<<(BLEU*8));
	data->colors[1] = (0x48<<(ROUGE*8))|(0x2c<<(VERT*8))|(0x6f<<(BLEU*8));
	data->colors[2] = (0x58<<(ROUGE*8))|(0x3c<<(VERT*8))|(0x0f<<(BLEU*8));
	data->colors[3] = (0x87<<(ROUGE*8))|(0x55<<(VERT*8))|(0x74<<(BLEU*8));
	tentacle_new(data);

	_this->params = &data->params;
	_this->fx_data = (void*)data;
}

static void tentacle_fx_apply(VisualFX *_this, Pixel *src, Pixel *dest, PluginInfo *goomInfo)
{
  TentacleFXData *data = (TentacleFXData*)_this->fx_data;
  if (BVAL(data->enabled_bp)) {
    tentacle_update(goomInfo, dest, src, goomInfo->screen.width,
                    goomInfo->screen.height, goomInfo->sound.samples,
                    (float)goomInfo->sound.accelvar,
                    goomInfo->curGState->drawTentacle, data);
  }
}

static void tentacle_fx_free(VisualFX *_this) {
        TentacleFXData *data = (TentacleFXData*)_this->fx_data;
        free(data->params.params);
	tentacle_free(data);
	free(_this->fx_data);
}

VisualFX tentacle_fx_create(void) {
	VisualFX fx;
	fx.init = tentacle_fx_init;
	fx.apply = tentacle_fx_apply;
	fx.free = tentacle_fx_free;
	return fx;
}

/* ----- */

static void tentacle_free (TentacleFXData *data) {
	/* TODO : un vrai FREE GRID!! */
        int tmp;
        for (tmp=0;tmp<nbgrid;tmp++){
            grid3d *g = data->grille[tmp];
            free (g->surf.vertex);
            free (g->surf.svertex);
            free (g->proj);
            free (g);
        }
	free (data->vals);
}

static void tentacle_new (TentacleFXData *data) {
	int tmp;
	int cols;
	int rows;

	v3d center = {0,-17.0,0};
	/* calloc so the wave-mode temporal smoothing starts from zeros */
	data->vals = (float*)calloc(definitionx + 20, sizeof(float));

	/* randomize the grid density (rows/columns) between the min and the
	 * current max so every show has a different resolution */
	cols = TENTACLE_GRID_MIN + rand() % (definitionx - TENTACLE_GRID_MIN + 1);
	rows = TENTACLE_GRID_MIN + rand() % (definitionz - TENTACLE_GRID_MIN + 1);

	for (tmp=0;tmp<nbgrid;tmp++) {
		int x,z;
		z = 45 + rand() % 30;
		x = 85 + rand() % 5;
		center.z = z;
		data->grille[tmp] = grid3d_new (x, cols, z, rows, center);
		center.y += 8;
	}
}

static void tentacle_recreate (TentacleFXData *data) {
	/* drop the previous grids and rebuild with fresh random density */
	tentacle_free(data);
	tentacle_new(data);
}

static inline unsigned char lighten (unsigned char value, float power)
{
	int val = value;
	float t = (float) val * log10(power) / 2.0;

	if (t > 0) {
		val = (int) t; /* (32.0f * log (t)); */
		if (val > 255)
			val = 255;
		if (val < 0)
			val = 0;
		return val;
	}
	else {
		return 0;
	}
}

static void lightencolor (int *col, float power)
{
	unsigned char *color;

	color = (unsigned char *) col;
	*color = lighten (*color, power);
	color++;
	*color = lighten (*color, power);
	color++;
	*color = lighten (*color, power);
	color++;
	*color = lighten (*color, power);
}

/* retourne x>>s , en testant le signe de x */
#define ShiftRight(_x,_s) ((_x<0) ? -(-_x>>_s) : (_x>>_s))

static int evolutecolor (unsigned int src,unsigned int dest,
                         unsigned int mask, unsigned int incr) {
	
	int color = src & (~mask);
	src &= mask;
	dest &= mask;

	if ((src!=mask)
			&&(src<dest))
		src += incr;

	if (src>dest)
		src -= incr;
	return (src&mask)|color;
}

static void pretty_move (PluginInfo *goomInfo, float cycle, float *dist, float *dist2, float *rotangle, TentacleFXData *fx_data) {

	float tmp;

	/* many magic numbers here... I don't really like that. */
	if (fx_data->happens)
		fx_data->happens -= 1;
	else if (fx_data->lock == 0) {
		if (fx_data->mode == TENTACLE_MODE_WAVE) {
			/* beat-synced only: a sweep fires on a goom, never on a timer */
			if (goomInfo->sound.timeSinceLastGoom == 0)
				fx_data->happens = TENTACLE_WAVE_SWEEP_MIN
					+ goom_irand(goomInfo->gRandom,TENTACLE_WAVE_SWEEP_RANGE);
		}
		else
			fx_data->happens = goom_irand(goomInfo->gRandom,200)?0:100+goom_irand(goomInfo->gRandom,60);
		fx_data->lock = fx_data->happens * 3 / 2;
	}
	else fx_data->lock --;

	tmp = fx_data->happens?8.0f:0;
	*dist2 = fx_data->distt2 = (tmp + 15.0f*fx_data->distt2)/16.0f;

	tmp = 30+D-90.0f*(1.0f+sin(cycle*19/20));
	if (fx_data->happens)
		tmp *= 0.6f;

	*dist = fx_data->distt = (tmp + 3.0f*fx_data->distt)/4.0f;

	if (!fx_data->happens){
		tmp = M_PI*sin(cycle)/32+3*M_PI/2;
	}
	else {
		fx_data->rotation = goom_irand(goomInfo->gRandom,500)?fx_data->rotation:goom_irand(goomInfo->gRandom,2);
		if (fx_data->rotation)
			cycle *= 2.0f*M_PI;
		else
			cycle *= -1.0f*M_PI;
		tmp = cycle - (M_PI*2.0) * floor(cycle/(M_PI*2.0));
	}

	if (abs(tmp-fx_data->rot) > abs(tmp-(fx_data->rot+2.0*M_PI))) {
		fx_data->rot = (tmp + 15.0f*(fx_data->rot+2*M_PI)) / 16.0f;
		if (fx_data->rot>2.0*M_PI)
			fx_data->rot -= 2.0*M_PI;
		*rotangle = fx_data->rot;
	}
	else if (abs(tmp-fx_data->rot) > abs(tmp-(fx_data->rot-2.0*M_PI))) {
		fx_data->rot = (tmp + 15.0f*(fx_data->rot-2.0*M_PI)) / 16.0f;
		if (fx_data->rot<0.0f)
			fx_data->rot += 2.0*M_PI;
		*rotangle = fx_data->rot;
	}
	else
		*rotangle = fx_data->rot = (tmp + 15.0f*fx_data->rot) / 16.0f;
}

static void tentacle_update(PluginInfo *goomInfo, Pixel *buf, Pixel *back, int W, int H,
                     short data[2][512], float rapport, int drawit, TentacleFXData *fx_data) {
	
	int tmp;
	int tmp2;

	int color;
	int colorlow;

	float dist,dist2,rotangle;
	float flow;
	float fade;
	float soundFactor;
	float surge;
	float rapportBase;

	/* Rebuild the grids with fresh random density each time the tentacle
	 * shows (rising edge of drawit), and pick the mode to run. */
	if (drawit && !fx_data->prevDrawit) {
		tentacle_recreate(fx_data);
		/* mix the modes: WAVE is more likely on fast music */
		if (goomInfo->sound.speedvar > TENTACLE_SPEED_THRESHOLD)
			fx_data->mode = (goom_irand(goomInfo->gRandom,100) < TENTACLE_WAVE_PROB_FAST)
				? TENTACLE_MODE_WAVE : TENTACLE_MODE_CLASSIC;
		else
			fx_data->mode = (goom_irand(goomInfo->gRandom,100) < TENTACLE_WAVE_PROB_SLOW)
				? TENTACLE_MODE_WAVE : TENTACLE_MODE_CLASSIC;
	}
	fx_data->prevDrawit = drawit;

	/* Fade envelope: ramps toward 1 while the tentacle should be shown,
	 * toward 0 otherwise, always at the fixed slow rate. */
	fade = (float)IVAL(fx_data->fade_p) / 100.0f;
	if (drawit)
		fx_data->fadeEnv += TENTACLE_FADE_STEP;
	else
		fx_data->fadeEnv -= TENTACLE_FADE_STEP;
	if (fx_data->fadeEnv > 1.0f)
		fx_data->fadeEnv = 1.0f;
	if (fx_data->fadeEnv < 0.0f)
		fx_data->fadeEnv = 0.0f;

	if (fx_data->fadeEnv > 0.0f) {
		/* brightness pulse */
		fx_data->lig += fx_data->ligs;
		if ((fx_data->lig>10.0f) | (fx_data->lig<1.1f)) fx_data->ligs = -fx_data->ligs;

		if ((fx_data->lig<6.3f)&&(goom_irand(goomInfo->gRandom,30)==0))
			fx_data->dstcol=goom_irand(goomInfo->gRandom,NB_TENTACLE_COLORS);

		fx_data->col = evolutecolor(fx_data->col,fx_data->colors[fx_data->dstcol],0xff,0x01);
		fx_data->col = evolutecolor(fx_data->col,fx_data->colors[fx_data->dstcol],0xff00,0x0100);
		fx_data->col = evolutecolor(fx_data->col,fx_data->colors[fx_data->dstcol],0xff0000,0x010000);
		fx_data->col = evolutecolor(fx_data->col,fx_data->colors[fx_data->dstcol],0xff000000,0x01000000);

		color = fx_data->col;
		colorlow = fx_data->col;

		soundFactor = 1.0f + fade * TENTACLE_FADE_SOUND_MAX * goomInfo->sound.volume;
		/* lighten() maps power->brightness via log10(power), so raise the
		 * power to fadeEnv (powf) instead of multiplying: log10(k^fadeEnv)
		 * = fadeEnv*log10(k) makes perceived brightness exactly linear in
		 * the envelope, removing the dead zone and the fast pop-in. */
		lightencolor(&color,powf((fx_data->lig * 2.0f + 2.0f) * soundFactor, fx_data->fadeEnv));
		lightencolor(&colorlow,powf(((fx_data->lig/3.0f)+0.67f) * soundFactor, fx_data->fadeEnv));

		rapport = 1.0f + 2.0f * (rapport - 1.0f);
		rapport *= 1.2f;
		rapportBase = rapport;   /* normalized energy for the camera surge */
		/* widen the loud end in wave mode so the cap can actually bind */
		if (fx_data->mode == TENTACLE_MODE_WAVE)
			rapport *= TENTACLE_WAVE_GAIN;
		{
			float rapportMax = (fx_data->mode == TENTACLE_MODE_WAVE)
				? TENTACLE_WAVE_RAPPORT_MAX : TENTACLE_CLASSIC_RAPPORT_MAX;
			if (rapport > rapportMax)
				rapport = rapportMax;
		}

		pretty_move (goomInfo, fx_data->cycle, &dist, &dist2, &rotangle, fx_data);
		/* wave mode: pull the camera in on loud hits, back out on quiet */
		if (fx_data->mode == TENTACLE_MODE_WAVE) {
			float t = (rapportBase + 1.2f) / 2.4f;   /* 0 quiet .. 1 loud */
			surge = TENTACLE_WAVE_SURGE_MAX
				- (TENTACLE_WAVE_SURGE_MAX - TENTACLE_WAVE_SURGE_MIN) * t;
			dist *= surge;
		}

		if (fx_data->mode == TENTACLE_MODE_WAVE) {
			/* one smoothed curtain shared by all grids (cols are uniform
			 * per show): fill + smooth once so the blend applies once */
			int gridCols = fx_data->grille[0]->defx;
			for (tmp2=0;tmp2<gridCols;tmp2++) {
				float val = (float)(ShiftRight(data[0][(tmp2 * 512) / gridCols],10)) * rapport;
				fx_data->vals[tmp2] += TENTACLE_WAVE_SMOOTH * (val - fx_data->vals[tmp2]);
			}
			for (tmp=0;tmp<nbgrid;tmp++)
				grid3d_update (fx_data->grille[tmp], rotangle, fx_data->vals, dist2);
		}
		else {
			/* classic keeps an independent random waveform per grid */
			for (tmp=0;tmp<nbgrid;tmp++) {
				int gridCols = fx_data->grille[tmp]->defx;
				for (tmp2=0;tmp2<gridCols;tmp2++)
					fx_data->vals[tmp2] = (float)(ShiftRight(data[0][goom_irand(goomInfo->gRandom,511)],10)) * rapport;
				grid3d_update (fx_data->grille[tmp], rotangle, fx_data->vals, dist2);
			}
		}
		/* drag the tentacle skin with the transformation buffer's velocity
		 * field: screen-space displacement of each projected vertex */
		flow = (float)IVAL(fx_data->flow_coupling_p) / 100.0f * TENTACLE_FLOW_FORCE;
		fx_data->cycle+=0.01f;
		for (tmp=0;tmp<nbgrid;tmp++)
			grid3d_draw (goomInfo, fx_data->grille[tmp],color,colorlow,dist,flow,buf,back,W,H);
	}
	else {
		fx_data->lig = 1.05f;
		if (fx_data->ligs < 0.0f)
			fx_data->ligs = -fx_data->ligs;
		pretty_move (goomInfo, fx_data->cycle, &dist, &dist2, &rotangle, fx_data);
		fx_data->cycle+=0.1f;
		if (fx_data->cycle > 1000)
			fx_data->cycle = 0;
	}
}
