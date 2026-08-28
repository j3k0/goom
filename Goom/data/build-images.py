#!/usr/bin/python

from gtimagelib import *

startResourceFile()

makeAllSizes(STD_SIZE, kBlack, "gui", "menu", "background", 1024, 768)
makeAllSizes(STD_SIZE, kBlack, "gui", "menu", "background-clean", 1024, 768)
makeAllSizes(STD_SIZE, kBlack, "gui", "menu", "button-hover", 310, 76)
makeAllSizes(STD_SIZE, kBlack, "gui", "menu", "button-normal", 310, 76)
makeAllSizes(STD_SIZE, kBlack, "gui", "menu", "button-pressed", 310, 76)
makeAllSizes(STD_SIZE, kBlack, "gui", "menu", "slider-empty", 22, 345)
makeAllSizes(STD_SIZE, kBlack, "gui", "menu", "slider-bar", 22, 345)

makeAllSizes(STD_SIZE, kBlack, "gui", "logo", "fovea", 256)

endResourceFile()
