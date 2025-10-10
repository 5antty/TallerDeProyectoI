# Ruta base de LVGL
LVGL_DIR       := $(PROJECT_DIR)/libs
LVGL_DIR_NAME  := lvgl

# Archivos fuente comunes de LVGL
include $(LVGL_DIR)/$(LVGL_DIR_NAME)/src/core/lv_core.mk
include $(LVGL_DIR)/$(LVGL_DIR_NAME)/src/draw/lv_draw.mk
include $(LVGL_DIR)/$(LVGL_DIR_NAME)/src/hal/lv_hal.mk
include $(LVGL_DIR)/$(LVGL_DIR_NAME)/src/misc/lv_misc.mk
include $(LVGL_DIR)/$(LVGL_DIR_NAME)/src/widgets/lv_widgets.mk
include $(LVGL_DIR)/$(LVGL_DIR_NAME)/src/font/lv_font.mk
include $(LVGL_DIR)/$(LVGL_DIR_NAME)/src/extra/lv_extra.mk

# Incluye también el header global de LVGL
CFLAGS += -I$(LVGL_DIR)/$(LVGL_DIR_NAME)/src
