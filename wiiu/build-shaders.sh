#!/bin/bash

# to build shaders you need to place a copy of latte-assembler into the current directory
# latte-assembler is part of decaf-emu <https://github.com/decaf-emu/decaf-emu>

# font_texture
./latte-assembler assemble --vsh=font_texture.vsh --psh=font_texture.psh data/font_texture.gsh
