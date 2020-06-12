/*
    This file is part of drmEasy.
    Copyright (C) 2020 ReimuNotMoe

    This program is free software: you can redistribute it and/or modify
    it under the terms of the MIT License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "drmEasy.hpp"

int main() {
	drmEasy::Device dev("/dev/dri/card0");

	dev.set_master();

	auto encoders = dev.encoders();
	auto connectors = dev.connectors();

	std::cout << "Connector count: " << connectors.size() << "\n";


	for (auto &it : connectors) {
		auto modes = it.modes();
		std::cout << "  Connector: id=" << it.id() << ", connection=" << it.connection() << ", mode_count=" << modes.size() << "\n";

		for (auto &itm : modes) {
			std::cout << "    Mode: " << itm.hdisplay << "x" << itm.vdisplay << " " << itm.vrefresh << "Hz\n";
		}
	}

	std::cout << "Encoder count: " << encoders.size() << "\n";

	for (auto &it : encoders) {
		std::cout << "  Encoder: id=" << it.id() << ", type=" << it.encoder_type() << ", crtc_id=" << it.crtc_id() << "\n";
	}

	auto crtc_orig = dev.get_crtc(encoders[0].crtc_id());

	std::cout << "id: " << crtc_orig.crtc_id << "\n"
		  << "fb_id: " << crtc_orig.fb_id << "\n"
		  << "x: " << crtc_orig.x << "\n"
		  << "y: " << crtc_orig.x << "\n"
		  << "mode_valid: " << crtc_orig.mode_valid << "\n"

		  << "\n";

	auto &cur_mode = crtc_orig.mode;

	auto fb_white = dev.create_framebuffer(cur_mode.hdisplay, cur_mode.vdisplay);
	memset(fb_white.data(), 255, cur_mode.hdisplay * cur_mode.vdisplay * 4);
	auto fb_black = dev.create_framebuffer(cur_mode.hdisplay, cur_mode.vdisplay);
	memset(fb_black.data(), 0, cur_mode.hdisplay * cur_mode.vdisplay * 4);

	auto crtc_new = crtc_orig;

	for (uint32_t i=0; i<20; i++) {
		if (i % 2)
			crtc_new.fb_id = fb_white.id();
		else
			crtc_new.fb_id = fb_black.id();

		dev.set_crtc(crtc_new, connectors[0]);

		usleep(100 * 1000);
	}

	dev.set_crtc(crtc_orig, connectors[0]);

	return 0;
}

