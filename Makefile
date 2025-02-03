# SPDX-License-Identifier: CC0-1.0
#
# SPDX-FileContributor: Antonio Niño Díaz, 2023

BLOCKSDS	?= /opt/blocksds/core

# User config

NAME		:= neverball-nds
GAME_TITLE	:= Neverball DS
GAME_ICON   := icon.png
#NITROFSDIR	:= nitro

include $(BLOCKSDS)/sys/default_makefiles/rom_arm9arm7/Makefile
