Import("env")

import subprocess

revision = subprocess.check_output(["git", "describe", "--tags", "--always", "--dirty"]).strip().decode("utf-8")
branch_path = subprocess.check_output(["git", "rev-parse", "--symbolic-full-name", "HEAD"]).rstrip().split(b"/")

env.Replace(PROGNAME="firmware_" + branch_path[-1].decode() + "-" + revision)
env.Replace(ESP32_SPIFFS_IMAGE_NAME="filesystem")

print("-DGIT_REV='\"%s\"'" % revision)
print("-DGIT_BRANCH='\"%s\"'" % branch_path[-1].decode())