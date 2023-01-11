/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>

#include "filters.h"

static __inline__ void flipVertical(uint8_t * source, int srcWidth, int srcHeight, uint8_t * destination,
        int dstWidth __unused, int dstHeight __unused) {
    //Vertical
    size_t cpy_bytes = sizeof(char) * 4;
    int width = cpy_bytes * srcWidth;
    int length = srcHeight;
    int total = length * width;
    size_t bytes_to_copy = sizeof(char) * width;
    int i = 0;
    int temp = total - width;
    for (i = 0; i < total; i += width) {
        memcpy(destination + temp - i, source + i, bytes_to_copy);
    }
}

static __inline__ void flipHorizontal(uint8_t * source, int srcWidth, int srcHeight,
        uint8_t * destination, int dstWidth __unused, int dstHeight __unused) {
    //Horizontal
    size_t cpy_bytes = sizeof(char) * 4;
    int width = cpy_bytes * srcWidth;
    int length = srcHeight;
    int total = length * width;
    int i = 0;
    int j = 0;
    int temp = 0;
    for (i = 0; i < total; i+= width) {
        temp = width + i - cpy_bytes;
        for (j = 0; j < width; j+=cpy_bytes) {
            memcpy(destination + temp - j, source + i + j, cpy_bytes);
        }
    }
}

static __inline__ void flip_fun(int flip, uint8_t * source, int srcWidth, int srcHeight, uint8_t * destination, int dstWidth, int dstHeight){
    int horiz = (flip & 1) != 0;
    int vert = (flip & 2) != 0;
    if (horiz && vert){
        int arr_len = dstWidth * dstHeight * sizeof(char) * 4;
        uint8_t* temp = (uint8_t *) malloc(arr_len);
        flipHorizontal(source, srcWidth, srcHeight, temp, dstWidth, dstHeight);
        flipVertical(temp, dstWidth, dstHeight, destination, dstWidth, dstHeight);
        free(temp);
        return;
    }
    if (horiz){
        flipHorizontal(source, srcWidth, srcHeight, destination, dstWidth, dstHeight);
        return;
    }
    if (vert){
        flipVertical(source, srcWidth, srcHeight, destination, dstWidth, dstHeight);
        return;
    }
}

//90 CCW (opposite of what's used in UI?)
static __inline__ void rotate90(uint8_t * source, int srcWidth, int srcHeight, uint8_t * destination,
        int dstWidth __unused, int dstHeight __unused) {
    size_t cpy_bytes = sizeof(char) * 4;
    int width = cpy_bytes * srcWidth;
    int length = srcHeight;
    for (size_t j = 0; j < length * cpy_bytes; j+= cpy_bytes){
        for (int i = 0; i < width; i+=cpy_bytes){
            int column_disp = (width - cpy_bytes - i) * length;
            int row_disp = j;
            memcpy(destination + column_disp + row_disp , source + j * srcWidth + i, cpy_bytes);
        }
    }
}

static __inline__ void rotate180(uint8_t * source, int srcWidth, int srcHeight, uint8_t * destination, int dstWidth, int dstHeight){
    flip_fun(3, source, srcWidth, srcHeight, destination, dstWidth, dstHeight);
}

static __inline__ void rotate270(uint8_t * source, int srcWidth, int srcHeight, uint8_t * destination, int dstWidth, int dstHeight){
    rotate90(source, srcWidth, srcHeight, destination, dstWidth, dstHeight);
    flip_fun(3, destination, dstWidth, dstHeight, destination, dstWidth, dstHeight);
}

// rotate == 1 is 90 degrees, 2 is 180, 3 is 270 (positive is CCW).
static __inline__ void rotate_fun(int rotate, uint8_t * source, int srcWidth, int srcHeight, uint8_t * destination, int dstWidth, int dstHeight){
    switch( rotate )
    {
        case 1:
            rotate90(source, srcWidth, srcHeight, destination, dstWidth, dstHeight);
            break;
        case 2:
            rotate180(source, srcWidth, srcHeight, destination, dstWidth, dstHeight);
            break;
        case 3:
            rotate270(source, srcWidth, srcHeight, destination, dstWidth, dstHeight);
            break;
        default:
            break;
    }
}

static __inline__ void crop(uint8_t * source, int srcWidth, int srcHeight, uint8_t * destination, int dstWidth, int dstHeight, int offsetWidth, int offsetHeight){
    size_t cpy_bytes = sizeof(char) * 4;
    int row_width = cpy_bytes * srcWidth;
    int new_row_width = cpy_bytes * dstWidth;
    if ((srcWidth > dstWidth + offsetWidth) || (srcHeight > dstHeight + offsetHeight)){
        return;
    }
    int j = 0;
    for (j = offsetHeight; j < offsetHeight + dstHeight; j++){
        memcpy(destination + (j - offsetHeight) * new_row_width, source + j * row_width + offsetWidth * cpy_bytes, cpy_bytes * dstWidth );
    }
}

void JNIFUNCF(ImageFilterGeometry, nativeApplyFilterFlip, jobject src, jint srcWidth, jint srcHeight, jobject dst, jint dstWidth, jint dstHeight, jint flip) {
    uint8_t* destination = 0;
    uint8_t* source = 0;
    if (srcWidth != dstWidth || srcHeight != dstHeight) {
        return;
    }
    AndroidBitmap_lockPixels(env, src, (void**) &source);
    AndroidBitmap_lockPixels(env, dst, (void**) &destination);
    flip_fun(flip, source, srcWidth, srcHeight, destination, dstWidth, dstHeight);
    AndroidBitmap_unlockPixels(env, dst);
    AndroidBitmap_unlockPixels(env, src);
}

void JNIFUNCF(ImageFilterGeometry, nativeApplyFilterRotate, jobject src, jint srcWidth, jint srcHeight, jobject dst, jint dstWidth, jint dstHeight, jint rotate) {
    uint8_t* destination = 0;
    uint8_t* source = 0;
    AndroidBitmap_lockPixels(env, src, (void**) &source);
    AndroidBitmap_lockPixels(env, dst, (void**) &destination);
    rotate_fun(rotate, source, srcWidth, srcHeight, destination, dstWidth, dstHeight);
    AndroidBitmap_unlockPixels(env, dst);
    AndroidBitmap_unlockPixels(env, src);
}

void JNIFUNCF(ImageFilterGeometry, nativeApplyFilterCrop, jobject src, jint srcWidth, jint srcHeight, jobject dst, jint dstWidth, jint dstHeight, jint offsetWidth, jint offsetHeight) {
    uint8_t* destination = 0;
    uint8_t* source = 0;
    AndroidBitmap_lockPixels(env, src, (void**) &source);
    AndroidBitmap_lockPixels(env, dst, (void**) &destination);
    crop(source, srcWidth, srcHeight, destination, dstWidth, dstHeight, offsetWidth, offsetHeight);
    AndroidBitmap_unlockPixels(env, dst);
    AndroidBitmap_unlockPixels(env, src);
}

void JNIFUNCF(ImageFilterGeometry, nativeApplyFilterStraighten, jobject src, jint srcWidth __unused,
        jint srcHeight __unused, jobject dst, jint dstWidth, jint dstHeight,
        jfloat straightenAngle __unused) {
    uint8_t* destination = 0;
    uint8_t* source = 0;
    int len = dstWidth * dstHeight * 4;
    AndroidBitmap_lockPixels(env, src, (void**) &source);
    AndroidBitmap_lockPixels(env, dst, (void**) &destination);
    // TODO: implement straighten
    int i = 0;
    for (; i < len; i += 4) {
        destination[RED] = 128;
        destination[GREEN] = source[GREEN];
        destination[BLUE] = 128;
    }
    AndroidBitmap_unlockPixels(env, dst);
    AndroidBitmap_unlockPixels(env, src);
}

