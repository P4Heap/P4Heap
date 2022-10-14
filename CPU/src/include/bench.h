#pragma once
#ifndef __BENCH_H__

#define __BENCH_H__
#include "dataset.h"
#include "topkframework.h"
#include "sketch.h"

/**
 * @brief Test on a particular kind of framework
 * 
 * @param stream dataset 
 * @param sketch entity of a kind of framework 
 */
void test(Dataset& stream, TopKFramework& framework);

/**
 * @brief Test on a particular kind of sketch
 * 
 * @param stream dataset 
 * @param sketch entity of a kind of sketch 
 */
void test(Dataset& stream, BaseSketch& sketch);

/**
 * @brief Test on combination of a particular TopK framework and 
 * a particular kind of sketch
 * 
 * @param stream dataset
 * @param framework entity of a kind of TopK framework 
 * @param sketch entity of a kind of sketch 
 */
void test(Dataset& stream, TopKFramework& framework, BaseSketch& sketch);

#endif
