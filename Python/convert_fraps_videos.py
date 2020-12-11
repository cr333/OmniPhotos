import os
import subprocess

for video in [
    "KyotoShrines-00-translation/result.flv"
]:
    print(video)
    subprocess.check_call(["ffmpeg",
                           "-i", video,
                           #"-filter:v", "setpts=2.0*PTS", "-r", "30",
                           "-pix_fmt", "yuvj420p",  # yuv420p
                           "-c:v", "libx264", "-preset", "slow", "-vprofile", "baseline",
                           "-threads", "0",
                           "-b:v", "20M",
                           "-pass", "1",
                           "-y",
                           video + ".mp4"])
    subprocess.check_call(["ffmpeg",
                           "-i", video,
                           #"-filter:v", "setpts=2.0*PTS", "-r", "30",
                           "-pix_fmt", "yuvj420p",  # yuv420p
                           "-c:v", "libx264", "-preset", "slow", "-vprofile", "baseline",
                           "-threads", "0",
                           "-b:v", "20M",
                           "-pass", "2",
                           "-y",
                           video + ".mp4"])

os.remove("ffmpeg2pass-0.log")
os.remove("ffmpeg2pass-0.log.mbtree")
