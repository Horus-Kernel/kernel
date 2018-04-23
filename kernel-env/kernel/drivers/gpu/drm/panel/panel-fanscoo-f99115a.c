/*
 * Copyright (C) 2018 
 *
 * Author: Francisco Leonardo Jales Martins <leonardo@larces.uece.br>
 *
 * From internet archives, the panel for Frascoo Eletronic Technology, 2014 model is a
 * Frascoo model F99115A-V00, and its data sheet is proprietary.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/backlight.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/regulator/consumer.h>

#include <drm/drmP.h>
#include <drm/drm_crtc.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_panel.h>

#include <video/mipi_display.h>

static const char * const regulator_names[] = {
	"vdd",
	"vddio"
};

struct fet_panel {
	struct drm_panel base;
	struct mipi_dsi_device *dsi;

	struct regulator_bulk_data supplies[ARRAY_SIZE(regulator_names)];

	struct gpio_desc *reset_gpio;
	struct backlight_device *backlight;

	bool prepared;
	bool enabled;

	const struct drm_display_mode *mode;
};

static inline struct fet_panel *to_fet_panel(struct drm_panel *panel)
{
	return container_of(panel, struct fet_panel, base);
}

static int fet_panel_init(struct fet_panel *fet)
{
	struct mipi_dsi_device *dsi = fet->dsi;
	struct device *dev = &fet->dsi->dev;
	int ret;

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	ret = mipi_dsi_dcs_soft_reset(dsi);
	if (ret < 0)
		return ret;

	usleep_range(10000, 20000);

	ret = mipi_dsi_dcs_write(dsi, 0xB0, (u8[]){0x04}, 1);
	if (ret < 0) {
		dev_err(dev, "failed to set mcap: %d\n", ret);
		return ret;
	}

	ret = mipi_dsi_dcs_write(dsi, 0xB3, (u8[]){0x00}, 1);
	if (ret < 0) {
		dev_err(dev, "failed to set Clock settings: %d\n", ret);
		return ret;
	}

	/* Interface setting, video mode */
	ret = mipi_dsi_dcs_write(dsi, 0xC4 ,(u8[])
				     { 0xBC, 0xB0, 0x81}, 3);
	if (ret < 0) {
		dev_err(dev, "failed to set display interface setting: %d\n"
			, ret);
		return ret;
	}

	ret = mipi_dsi_dcs_write(dsi, 0xB0, (u8[]){0x04}, 1);
	if (ret < 0) {
		dev_err(dev, "failed to set default values for mcap: %d\n"
			, ret);
		return ret;
	}

	ret = mipi_dsi_dcs_write(dsi, 0xB6, (u8[]){0x32}, 1);
	if (ret < 0) {
		dev_err(dev, "failed to set default values for DSI Setting(B6h): %d\n"
			, ret);
		return ret;
	}

	ret = mipi_dsi_dcs_write(dsi, 0xE5, (u8[]){0x02}, 1);
	if (ret < 0) {
		dev_err(dev, "failed to set default values for Register write Control(E5h): %d\n"
			, ret);
		return ret;
	}

	ret = mipi_dsi_dcs_write(dsi, 0xC0, (u8[]){0x23, 0xB2, 0x0D, 0x10, 0x02, 0x80}, 6);
	if (ret < 0) {
		dev_err(dev, "failed to set default values for Display setting (C0h): %d\n"
			, ret);
		return ret;
	}

	ret = mipi_dsi_dcs_write(dsi, 0xC1, (u8[]){0xA0, 0x70, 0x60, 0x60, 0x01, 0x01, 0xF0}, 7);
	if (ret < 0) {
		dev_err(dev, "failed to set default values for DisplaY h-Timing Setting(C1h): %d\n"
			, ret);
		return ret;
	}

	ret = mipi_dsi_dcs_write(dsi, 0xC5, (u8[]){0x07, 0x00, 0x37}, 3);
	if (ret < 0) {
		dev_err(dev, "failed to set default values for Display V-Timing setting (C5h): %d\n"
			, ret);
		return ret;
	}

	ret = mipi_dsi_dcs_write(dsi, 0xC6, (u8[]){0x21}, 1);
	if (ret < 0) {
		dev_err(dev, "failed to set default values for Panel Drive Setting (C6h): %d\n"
			, ret);
		return ret;
	}

	ret = mipi_dsi_dcs_write(dsi, 0xD0, (u8[]){0x05, 0x8B, 0x42}, 3);
	if (ret < 0) {
		dev_err(dev, "failed to set default values for Power Setting of VCI, VGH, VGL(D0h): %d\n"
			, ret);
		return ret;
	}

	ret = mipi_dsi_dcs_write(dsi, 0xD1, (u8[]){0x03}, 1);
	if (ret < 0) {
		dev_err(dev, "failed to set default values for Power Setting of VCL (D1h): %d\n"
			, ret);
		return ret;
	}

	ret = mipi_dsi_dcs_write(dsi, 0xD2, (u8[]){0x81, 0x1F}, 2);
	if (ret < 0) {
		dev_err(dev, "failed to set default values for Power Setting of External Booster (D2h): %d\n"
			, ret);
		return ret;
	}

	ret = mipi_dsi_dcs_write(dsi, 0xD3, (u8[]){0x11, 0x33}, 2);
	if (ret < 0) {
		dev_err(dev, "failed to set default values for Power Setting of Internal circuit (D3h): %d\n"
			, ret);
		return ret;
	}

	ret = mipi_dsi_dcs_write(dsi, 0xD4, (u8[]){0x4A}, 1);
	if (ret < 0) {
		dev_err(dev, "failed to set default values for VCOMDC Setting (D4h): %d\n"
			, ret);
		return ret;
	}

	ret = mipi_dsi_dcs_write(dsi, 0xD5, (u8[]){0x32, 0x32}, 2);
	if (ret < 0) {
		dev_err(dev, "failed to set default values for VPLVL/VNLVL Setting (D5h): %d\n"
			, ret);
		return ret;
	}

	ret = mipi_dsi_dcs_write(dsi, 0xD6, (u8[]){0x01}, 1);
	if (ret < 0) {
		dev_err(dev, "failed to set default values for Abnormal off Sequence Control (D6h): %d\n"
			, ret);
		return ret;
	}

	ret = mipi_dsi_dcs_write(dsi, 0xC8, (u8[]){0x61, 0x4E, 0x42, 0xAA, 0x90, 0x5A, 0x6B,
						   0xAD, 0xB5, 0xD6, 0x5A, 0x6B, 0x85, 0x51, 
						   0xCA, 0x08, 0x6B, 0xAD, 0xB5, 0xD6, 0x5A,
						   0x6B, 0xAD, 0x95, 0x14, 0x85, 0xD2, 0x31}, 28);
	if (ret < 0) {
		dev_err(dev, "failed to set default values for GOUT Pin Assignment (C8h): %d\n"
			, ret);
		return ret;
	}

	ret = mipi_dsi_dcs_write(dsi, 0xCA, (u8[]){0x00, 0x06, 0x0A, 0x0F, 0x17,
						   0x1D, 0x22, 0x22, 0x1F, 0x1D, 
						   0x1B, 0x17, 0x13, 0x0C, 0x00, 
						   0x00, 0x06, 0x0A, 0x0F, 0x17, 
						   0x1D, 0x22, 0x22, 0x1F, 0x1D, 
						   0x1B, 0x17, 0x13, 0x0C, 0x00}, 30);
	if (ret < 0) {
		dev_err(dev, "failed to set default values for Gamma Setting Common (CAh): %d\n"
			, ret);
		return ret;
	}

	ret = mipi_dsi_dcs_exit_sleep_mode(dsi);
	if (ret < 0) {
		dev_err(dev, "failed to set exit sleep mode: %d\n", ret);
		return ret;
	}

	msleep(120);

	return 0;

}

static int fet_panel_on(struct fet_panel *fet)
{
	struct mipi_dsi_device *dsi = fet->dsi;
	struct device *dev = &fet->dsi->dev;
	int ret;

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	ret = mipi_dsi_dcs_set_display_on(dsi);
	if (ret < 0)
		dev_err(dev, "failed to set display on: %d\n", ret);

	mdelay(10);

	return ret;
}

static void fet_panel_off(struct fet_panel *fet)
{
	struct mipi_dsi_device *dsi = fet->dsi;
	struct device *dev = &fet->dsi->dev;
	int ret;

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	ret = mipi_dsi_dcs_write(dsi, 0x11, (u8[]){ 0x00 }, 1);
	if(ret < 0)
		dev_err(dev, "failed to write sleep mode off: %d\n", ret);

	ret = mipi_dsi_dcs_set_display_off(dsi);
	if (ret < 0)
		dev_err(dev, "failed to set display off: %d\n", ret);

	ret = mipi_dsi_dcs_enter_sleep_mode(dsi);
	if (ret < 0)
		dev_err(dev, "failed to enter sleep mode: %d\n", ret);

	msleep(100);
}

static int fet_panel_disable(struct drm_panel *panel)
{
	struct fet_panel *fet = to_fet_panel(panel);

	if (!fet->enabled)
		return 0;

	fet->backlight->props.power = FB_BLANK_POWERDOWN;
	backlight_update_status(fet->backlight);

	fet->enabled = false;

	return 0;
}

static int fet_panel_unprepare(struct drm_panel *panel)
{
	struct fet_panel *fet = to_fet_panel(panel);
	struct device *dev = &fet->dsi->dev;
	int ret;

	if (!fet->prepared)
		return 0;

	fet_panel_off(fet);

	ret = regulator_bulk_disable(ARRAY_SIZE(fet->supplies), fet->supplies);
	if (ret < 0)
		dev_err(dev, "regulator disable failed, %d\n", ret);

	gpiod_set_value(fet->reset_gpio, 0);

	fet->prepared = false;

	return 0;
}

static int fet_panel_prepare(struct drm_panel *panel)
{
	struct fet_panel *fet = to_fet_panel(panel);
	struct device *dev = &fet->dsi->dev;
	int ret;

	if (fet->prepared)
		return 0;

	ret = regulator_bulk_enable(ARRAY_SIZE(fet->supplies), fet->supplies);
	if (ret < 0) {
		dev_err(dev, "regulator enable failed, %d\n", ret);
		return ret;
	}

	msleep(20);

	gpiod_set_value(fet->reset_gpio, 1);
	usleep_range(10, 20);

	ret = fet_panel_init(fet);
	if (ret < 0) {
		dev_err(dev, "failed to init panel: %d\n", ret);
		goto poweroff;
	}

	ret = fet_panel_on(fet);
	if (ret < 0) {
		dev_err(dev, "failed to set panel on: %d\n", ret);
		goto poweroff;
	}

	fet->prepared = true;

	return 0;

poweroff:
	ret = regulator_bulk_disable(ARRAY_SIZE(fet->supplies), fet->supplies);
	if (ret < 0)
		dev_err(dev, "regulator disable failed, %d\n", ret);

	gpiod_set_value(fet->reset_gpio, 0);

	return ret;
}

static int fet_panel_enable(struct drm_panel *panel)
{
	struct fet_panel *fet = to_fet_panel(panel);

	if (fet->enabled)
		return 0;

	fet->backlight->props.power = FB_BLANK_UNBLANK;
	backlight_update_status(fet->backlight);

	fet->enabled = true;

	return 0;
}

static const struct drm_display_mode default_mode = {
		.clock = 63037,
		.hdisplay = 720, //x
		.hsync_start = 720 + 40, // x + hfp 
		.hsync_end = 720 + 40 + 12, // x + hfp + hsl
		.htotal = 720 + 40 + 12 + 30, // x + hfp + hsl + hpb
		.vdisplay = 1280, // y
		.vsync_start = 1280 + 18, // y + vfp
		.vsync_end = 1280 + 18 + 3, // y + vfp + vsl
		.vtotal = 1280 + 18 + 3 + 9, // y + vfp + vsl + vbp
		.vrefresh = 60,
		.flags = 0,
};

static int fet_panel_get_modes(struct drm_panel *panel)
{
	struct drm_display_mode *mode;
	struct fet_panel *fet = to_fet_panel(panel);
	struct device *dev = &fet->dsi->dev;

	mode = drm_mode_duplicate(panel->drm, &default_mode);
	if (!mode) {
		dev_err(dev, "failed to add mode %ux%ux@%u\n",
			default_mode.hdisplay, default_mode.vdisplay,
			default_mode.vrefresh);
		return -ENOMEM;
	}

	drm_mode_set_name(mode);

	drm_mode_probed_add(panel->connector, mode);

	panel->connector->display_info.width_mm = 63;
	panel->connector->display_info.height_mm = 111;

	return 1;
}

static int dsi_dcs_bl_get_brightness(struct backlight_device *bl)
{
	struct mipi_dsi_device *dsi = bl_get_data(bl);
	int ret;
	u16 brightness = bl->props.brightness;

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	ret = mipi_dsi_dcs_get_display_brightness(dsi, &brightness);
	if (ret < 0)
		return ret;

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	return brightness & 0xff;
}

static int dsi_dcs_bl_update_status(struct backlight_device *bl)
{
	struct mipi_dsi_device *dsi = bl_get_data(bl);
	int ret;

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	ret = mipi_dsi_dcs_set_display_brightness(dsi, bl->props.brightness);
	if (ret < 0)
		return ret;

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;
	
	return 0;
}

static const struct backlight_ops dsi_bl_ops = {
	.update_status = dsi_dcs_bl_update_status,
	.get_brightness = dsi_dcs_bl_get_brightness,
};

static struct backlight_device *
drm_panel_create_dsi_backlight(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct backlight_properties props;

	memset(&props, 0, sizeof(props));
	props.type = BACKLIGHT_RAW;
	props.brightness = 255;
	props.max_brightness = 255;

	return devm_backlight_device_register(dev, dev_name(dev), dev, dsi,
					      &dsi_bl_ops, &props);
}

static const struct drm_panel_funcs fet_panel_funcs = {
	.disable = fet_panel_disable,
	.unprepare = fet_panel_unprepare,
	.prepare = fet_panel_prepare,
	.enable = fet_panel_enable,
	.get_modes = fet_panel_get_modes,
};

static const struct of_device_id fet_of_match[] = {
	{ .compatible = "fet,f99115a", },
	{ }
};
MODULE_DEVICE_TABLE(of, fet_of_match);

static int fet_panel_add(struct fet_panel *fet)
{
	struct device *dev = &fet->dsi->dev;
	int ret;
	unsigned int i;

	fet->mode = &default_mode;

	for (i = 0; i < ARRAY_SIZE(fet->supplies); i++)
		fet->supplies[i].supply = regulator_names[i];

	ret = devm_regulator_bulk_get(dev, ARRAY_SIZE(fet->supplies),
				      fet->supplies);
	if (ret < 0) {
		dev_err(dev, "failed to init regulator, ret=%d\n", ret);
		return ret;
	}

	fet->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(fet->reset_gpio)) {
		ret = PTR_ERR(fet->reset_gpio);
		dev_err(dev, "cannot get reset-gpios %d\n", ret);
		return ret;
	}

	fet->backlight = drm_panel_create_dsi_backlight(fet->dsi);
	if (IS_ERR(fet->backlight)) {
		ret = PTR_ERR(fet->backlight);
		dev_err(dev, "failed to register backlight %d\n", ret);
		return ret;
	}

	drm_panel_init(&fet->base);
	fet->base.funcs = &fet_panel_funcs;
	fet->base.dev = &fet->dsi->dev;

	ret = drm_panel_add(&fet->base);

	return ret;
}

static void fet_panel_del(struct fet_panel *fet)
{
	if (fet->base.dev)
		drm_panel_remove(&fet->base);
}

static int fet_panel_probe(struct mipi_dsi_device *dsi)
{
	struct fet_panel *fet;
	int ret;

	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB888;
	/*SEE https://discuss.96boards.org/t/linux-mipi-dsi-panel-support/196/23 */
	dsi->mode_flags =  MIPI_DSI_MODE_VIDEO |
			  MIPI_DSI_CLOCK_NON_CONTINUOUS |
			  MIPI_DSI_MODE_VIDEO_BURST; 

	fet = devm_kzalloc(&dsi->dev, sizeof(*fet), GFP_KERNEL);
	if (!fet)
		return -ENOMEM;

	mipi_dsi_set_drvdata(dsi, fet);

	fet->dsi = dsi;

	ret = fet_panel_add(fet);
	if (ret < 0)
		return ret;

	return mipi_dsi_attach(dsi);
}

static int fet_panel_remove(struct mipi_dsi_device *dsi)
{
	struct fet_panel *fet = mipi_dsi_get_drvdata(dsi);
	int ret;

	ret = fet_panel_disable(&fet->base);
	if (ret < 0)
		dev_err(&dsi->dev, "failed to disable panel: %d\n", ret);

	ret = mipi_dsi_detach(dsi);
	if (ret < 0)
		dev_err(&dsi->dev, "failed to detach from DSI host: %d\n",
			ret);

	drm_panel_detach(&fet->base);
	fet_panel_del(fet);

	return 0;
}

static void fet_panel_shutdown(struct mipi_dsi_device *dsi)
{
	struct fet_panel *fet = mipi_dsi_get_drvdata(dsi);

	fet_panel_disable(&fet->base);
}

static struct mipi_dsi_driver fet_panel_driver = {
	.driver = {
		.name = "panel-fet-f99115a",
		.of_match_table = fet_of_match,
	},
	.probe = fet_panel_probe,
	.remove = fet_panel_remove,
	.shutdown = fet_panel_shutdown,
};
module_mipi_dsi_driver(fet_panel_driver);

MODULE_AUTHOR("Leonardo Jales <leonardo@larces.uece.br>");
MODULE_DESCRIPTION("FET F99115A RGB");
MODULE_LICENSE("GPL v2");
