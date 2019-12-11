#!/bin/sh

nc -l -k -w 5 4200 | tee -a log
