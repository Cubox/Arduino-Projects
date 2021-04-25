#!/bin/sh

nc -l -k -w 5 4202 | tee -a log2
