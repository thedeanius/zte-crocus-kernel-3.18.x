#
# Makefile for the input core drivers.
#

# Each configuration option enables a list of files.

obj-$(CONFIG_INPUT)		+= input-core.o
input-core-y := input.o input-compat.o input-mt.o ff-core.o

obj-$(CONFIG_INPUT_FF_MEMLESS)	+= ff-memless.o
obj-$(CONFIG_INPUT_POLLDEV)	+= input-polldev.o
obj-$(CONFIG_INPUT_SPARSEKMAP)	+= sparse-keymap.o
obj-$(CONFIG_INPUT_MATRIXKMAP)	+= matrix-keymap.o

obj-$(CONFIG_INPUT_MOUSEDEV)	+= mousedev.o
obj-$(CONFIG_INPUT_JOYDEV)	+= joydev.o
obj-$(CONFIG_INPUT_EVDEV)	+= evdev.o
obj-$(CONFIG_INPUT_EVBUG)	+= evbug.o

obj-$(CONFIG_INPUT_KEYBOARD)	+= keyboard/
obj-$(CONFIG_INPUT_MOUSE)	+= mouse/
obj-$(CONFIG_INPUT_JOYSTICK)	+= joystick/
obj-$(CONFIG_INPUT_TABLET)	+= tablet/
obj-$(CONFIG_INPUT_TOUCHSCREEN)	+= touchscreen/
obj-$(CONFIG_INPUT_MISC)	+= misc/

ifeq ($(ZTE_FINGERPRINT_MODE_3208),true)
obj-$(CONFIG_FINGERPRINT_GOODIX_GF3208)  += fingerprint_gf3208/
endif

ifeq ($(ZTE_FINGERPRINT_MODE_3228),true)
obj-$(CONFIG_FINGERPRINT_GOODIX_GF3228)  += fingerprint/goodix/
endif

ifeq ($(ZTE_FINGERPRINT_MODE_3238),true)
obj-$(CONFIG_FINGERPRINT_GOODIX_GF3238) += fingerprint/gf3238_driver/
endif

obj-$(CONFIG_INPUT_APMPOWER)	+= apm-power.o
obj-$(CONFIG_INPUT_KEYRESET)	+= keyreset.o
obj-$(CONFIG_INPUT_KEYCOMBO)	+= keycombo.o

#jiangfeng add for SX9500 20150226
obj-$(CONFIG_ZTE_INPUT_MISC) += ztemisc/
