#ifndef _IOS_HASH_H
#define _IOS_HASH_H

namespace ios_fc {

/**
 * Copyright (c) 2004, JC Hoelt.
 *
 * 2012/5/9: String values are now duplicated internally, to simplify higher level memory management.
 */

typedef struct IOS_HASH IosHash;

enum HashValueType {
    HASH_TYPE_POINTER,
    HASH_TYPE_STRING,
    HASH_TYPE_LONG,
    HASH_TYPE_INT,
    HASH_TYPE_FLOAT
};
    
typedef struct {
    HashValueType type;
    union {
        void *ptr;
        const char *str;
//        long  l;
        int   i;
        float f;
    } value;
} HashValue;

/* a hash map having a char* as key */
IosHash  *ios_hash_new       ();
void       ios_hash_free     (IosHash *gh);
HashValue *ios_hash_get      (IosHash *gh, const char *key);
void       ios_hash_put      (IosHash *gh, const char *key, HashValue value);
void       ios_hash_remove   (IosHash *gh, const char *key);

void ios_hash_put_int  (IosHash *_this,  const char *key, int i);
void ios_hash_put_float(IosHash *_this,  const char *key, float f);
    void ios_hash_put_ptr  (IosHash *_this,  const char *key, void *ptr);
void ios_hash_put_str  (IosHash *_this,  const char *key, const char *ptr);

class HashMapAction {
  public:
    virtual void action(HashValue *value) = 0;
    virtual void action(const char *key, HashValue *value) { action(value); }
    virtual ~HashMapAction() {};
};

// A Hashmap where keys are strings.
class HashMap {
    private:
        IosHash *hash;
    public:
        HashMap() : hash(ios_hash_new()) {}
        ~HashMap() { ios_hash_free(hash); }
        HashMap(const HashMap &h);
        HashMap& operator=(const HashMap& hmap);

        void put(const char *key, void *value) { ios_hash_put_ptr  (hash, key, value); }
        void put(const char *key, const char *value) { ios_hash_put_str  (hash, key, value); }
        void put(const char *key, int   value) { ios_hash_put_int  (hash, key, value); }
        void put(const char *key, float value) { ios_hash_put_float(hash, key, value); }
        void put(const char *key, HashValue value) { ios_hash_put(hash, key, value); }

        HashValue *get     (const char *key) const { return ios_hash_get(hash, key);      }
        void       remove  (const char *key)       { ios_hash_remove(hash, key);          }
    
        void forEach(HashMapAction *action);

        int getInt(const char *key, int defaultValue) {
            HashValue *ret = get(key);
            if ((ret == (HashValue*)0) || (ret->type != HASH_TYPE_INT))
                return defaultValue;
            else
                return ret->value.i;
        }
        
        int getFloat(const char *key, float defaultValue) {
            HashValue *ret = get(key);
            if ((ret == (HashValue*)0) || (ret->type != HASH_TYPE_FLOAT))
                return defaultValue;
            else
                return ret->value.f;
        }
};

} // namespace ios_fc

#endif /* _IOS_HASH_H */
