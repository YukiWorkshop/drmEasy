/*
    This file is part of drmEasy.
    Copyright (C) 2020 ReimuNotMoe

    This program is free software: you can redistribute it and/or modify
    it under the terms of the MIT License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <system_error>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cinttypes>

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <drm/drm.h>

#define THROW_SYSTEM_ERROR(str)		throw std::system_error(errno, std::system_category(), str)

namespace drmEasy {
	class Framebuffer {
	private:
		int fd_dri = -1;
		uint32_t fb_id = 0;
		uint32_t depth_ = 0;
		drm_mode_create_dumb fb_params;
		void *fb_data = nullptr;

		void drm_create_dumb(drm_mode_create_dumb& __params) const;
		void drm_destroy_dumb(drm_mode_destroy_dumb& __params) const;
		void drm_add_fb(drm_mode_fb_cmd& __params) const;
		void drm_rm_fb(uint32_t __fb_id) const;
		void drm_map_dumb(drm_mode_map_dumb& __params);

	public:
		Framebuffer(int __fd_dri, uint32_t __depth, drm_mode_create_dumb& __params);
		~Framebuffer();

		uint32_t id() const noexcept;
		const drm_mode_create_dumb& params() const noexcept;
		uint8_t *data() const noexcept;
	};

	class Encoder {
	private:
		int fd_dri = -1;
		uint32_t enc_id = 0;

		void drm_get_encoder(drm_mode_get_encoder& __enc) const;
	public:
		Encoder(int __fd_dri, uint32_t __enc_id) : fd_dri(__fd_dri), enc_id(__enc_id) {

		}

		uint32_t id() const noexcept;
		uint32_t encoder_type() const;
		uint32_t crtc_id() const;

	};

	class Connector {
	private:
		int fd_dri = -1;
		uint32_t conn_id = 0;

		void drm_get_connector(drm_mode_get_connector& __conn) const;
	public:
		Connector(int __fd_dri, uint32_t __conn_id) : fd_dri(__fd_dri), conn_id(__conn_id) {

		}

		uint32_t id() const noexcept;
		std::vector<drm_mode_modeinfo> modes() const;
		uint32_t connection() const;

	};

	class Device {
	private:
		int fd_dri = -1;
		std::string path_;

		void drm_get_resources(drm_mode_card_res& __res) const;
		void drm_get_crtc(drm_mode_crtc& __crtc) const;
		void drm_set_crtc(const drm_mode_crtc& __crtc) const;
	public:
		Device(const std::string& __path);
		~Device();

		void set_master(bool __enabled = true);

		std::vector<Connector> connectors() const;
		std::vector<Encoder> encoders() const;

		drm_mode_crtc get_crtc(uint32_t __crtc_id) const;

		void set_crtc(const drm_mode_crtc& __crtc);
		void set_crtc(uint32_t __crtc_id, uint32_t __fb_id, uint32_t __x, uint32_t __y,
			      uint32_t *__connector_ids, uint32_t __connector_count = 0);
		void set_crtc(uint32_t __crtc_id, uint32_t __fb_id, uint32_t __x, uint32_t __y,
			      uint32_t *__connector_ids, uint32_t __connector_count,
			      const drm_mode_modeinfo& __mode);
		void set_crtc(const drm_mode_crtc& __crtc, const Connector& __connector);
		void set_crtc(const drm_mode_crtc& __crtc, const Connector& __connector, const drm_mode_modeinfo& __mode);
		void set_crtc(uint32_t __crtc_id, const Framebuffer& __frame_buffer, const Connector& __connector);

		Framebuffer create_framebuffer(uint32_t __width, uint32_t __height, uint8_t __depth = 24, uint8_t __bpp = 32);

	};
};