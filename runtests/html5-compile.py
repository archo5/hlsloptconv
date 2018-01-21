#!/usr/bin/python3

import os, subprocess

here = os.path.dirname(os.path.realpath(__file__))
with open(os.path.join(here, "html5-stub.htm"), "r") as f:
	stub = f.read()

def ConvertShader(is_ps):
	return subprocess.check_output([
		os.path.join(here, "..", "hlsloptconv"),
		"-P",
		"-D" + ("PS=1" if is_ps else "VS=1"),
		"-f", "glsl_es_100",
		"-s", "pixel" if is_ps else "vertex",
		"-x", "jsstr",
		os.path.join(here, "html5-shader.hlsl"),
	]).decode("utf-8").replace("\r\n", "\n")

vshader = ConvertShader(False)
pshader = ConvertShader(True)

html = stub
html = html.replace("VERTEX_SHADER_STRING_POS", vshader)
html = html.replace("PIXEL_SHADER_STRING_POS", pshader)
with open(os.path.join(here, "gen-html5.htm"), "w") as f:
	f.write(html)
