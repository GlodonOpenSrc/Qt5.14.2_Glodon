#!/usr/bin/env python
# coding: utf-8
import numpy as np
import numpy.linalg as LA
import matplotlib.pyplot as plt
from scipy.ndimage import filters
from PIL import Image, ImageDraw


def MSE(blk1, blk2):
  return np.mean(
      LA.norm(
          np.array(blk1, dtype=np.int) - np.array(blk2, dtype=np.int), axis=2))


def drawMF(img, blk_sz, mf):
  img_rgba = img.convert('RGBA')
  mf_layer = Image.new(mode='RGBA', size=img_rgba.size, color=(0, 0, 0, 0))
  draw = ImageDraw.Draw(mf_layer)
  width = img_rgba.size[0]
  height = img_rgba.size[1]
  num_row = height // blk_sz
  num_col = width // blk_sz
  for i in xrange(num_row):
    left = (0, i * blk_sz)
    right = (width, i * blk_sz)
    draw.line([left, right], fill=(0, 0, 255, 255))
  for j in xrange(num_col):
    up = (j * blk_sz, 0)
    down = (j * blk_sz, height)
    draw.line([up, down], fill=(0, 0, 255, 255))
  for i in xrange(num_row):
    for j in xrange(num_col):
      center = (j * blk_sz + 0.5 * blk_sz, i * blk_sz + 0.5 * blk_sz)
      head = (center[0] + mf[i, j][1], center[1] + mf[i, j][0])
      draw.line([center, head], fill=(255, 0, 0, 255))
  return Image.alpha_composite(img_rgba, mf_layer)
