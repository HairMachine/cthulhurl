#!/bin/bash

gcc main.c dijkstra.c -Iinclude -L. -ltcod -ltcodxx -Wl,-rpath=. -Wall