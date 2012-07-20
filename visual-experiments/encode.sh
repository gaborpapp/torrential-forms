#!/bin/sh
sox capture.wav capture-trimmed.wav trim $1
rm capture.mp4
glc-play capture.glc -o - -y 1 | ffmpeg -i - -i capture-trimmed.wav -acodec libmp3lame -ab 128k -ac 2 -vcodec libx264 -vpre slow -ss 15 -t 87 capture.mp4
rm capture-trimmed.wav
