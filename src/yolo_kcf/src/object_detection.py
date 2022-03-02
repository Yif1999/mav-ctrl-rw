#!/usr/bin/env python3

import cv2 as cv
import sys
import numpy as np

def imcv2_recolor(im, a=.1):
    # t = [np.random.uniform()]
    # t += [np.random.uniform()]
    # t += [np.random.uniform()]
    # t = np.array(t) * 2. - 1.
    t = np.random.uniform(-1, 1, 3)

    # random amplify each channel
    im = im.astype(np.float)
    im *= (1 + t * a)
    mx = 255. * (1 + a)
    up = np.random.uniform(-1, 1)
    im = np.power(im / mx, 1. + up * .5)
    # return np.array(im * 255., np.uint8)
    return im

class object_detector:

    def __init__(self, model, cfg):
        self.model = model
        self.cfg = cfg
        self.framework = None
        self.load_model()

    def load_model(self):
        self.net = cv.dnn.readNetFromDarknet(self.cfg, self.model)
        self.framework = 'Darknet'

        self.net.setPreferableBackend(cv.dnn.DNN_BACKEND_DEFAULT)
        self.net.setPreferableTarget(cv.dnn.DNN_TARGET_OPENCL)


    def predict(self,frame):

        # Create a 4D blob from a frame.
        #blob = cv.dnn.blobFromImage(frame, 0.007843, (416, 416), 127.5, crop = False)
        blob = cv.dnn.blobFromImage(cv.resize(frame, (416, 416)), 0.003921, (416, 416), (0,0,0), swapRB=True,  crop=False)

        # Run a model
        self.net.setInput(blob)
        out = self.net.forward()
        
        return out
        
