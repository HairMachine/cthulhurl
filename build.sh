#!/bin/bash

gcc main.c -Iinclude -L. -ltcod -ltcodxx -Wl,-rpath=. -Wall