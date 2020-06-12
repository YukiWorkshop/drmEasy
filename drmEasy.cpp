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

using namespace drmEasy;

// Framebuffer

void Framebuffer::drm_create_dumb(drm_mode_create_dumb &__params) const {
	if (ioctl(fd_dri, DRM_IOCTL_MODE_CREATE_DUMB, &__params))
		THROW_SYSTEM_ERROR("DRM_IOCTL_MODE_CREATE_DUMB");
}

void Framebuffer::drm_destroy_dumb(drm_mode_destroy_dumb &__params) const {
	if (ioctl(fd_dri, DRM_IOCTL_MODE_DESTROY_DUMB, &__params))
		THROW_SYSTEM_ERROR("DRM_IOCTL_MODE_DESTROY_DUMB");
}

void Framebuffer::drm_add_fb(drm_mode_fb_cmd &__params) const {
	if (ioctl(fd_dri, DRM_IOCTL_MODE_ADDFB, &__params))
		THROW_SYSTEM_ERROR("DRM_IOCTL_MODE_ADDFB");
}

void Framebuffer::drm_rm_fb(uint32_t __fb_id) const {
	if (ioctl(fd_dri, DRM_IOCTL_MODE_RMFB, &__fb_id))
		THROW_SYSTEM_ERROR("DRM_IOCTL_MODE_RMFB");
}

void Framebuffer::drm_map_dumb(drm_mode_map_dumb &__params) {
	if (ioctl(fd_dri, DRM_IOCTL_MODE_MAP_DUMB, &__params))
		THROW_SYSTEM_ERROR("DRM_IOCTL_MODE_MAP_DUMB");

	fb_data = mmap(nullptr, fb_params.size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_dri, __params.offset);

	if (fb_data == MAP_FAILED)
		THROW_SYSTEM_ERROR("mmap");
}

Framebuffer::Framebuffer(int __fd_dri, uint32_t __depth, drm_mode_create_dumb &__params) : fd_dri(__fd_dri) {
	drm_create_dumb(__params);
	fb_params = __params;

	drm_mode_fb_cmd fc{};
	fc.width = fb_params.width;
	fc.height = fb_params.height;
	fc.bpp = fb_params.bpp;
	fc.pitch = fb_params.pitch;
	fc.depth = __depth;
	fc.handle = fb_params.handle;
	drm_add_fb(fc);

	drm_mode_map_dumb md{};
	md.handle = __params.handle;
	drm_map_dumb(md);

	fb_id = fc.fb_id;
}

Framebuffer::~Framebuffer() {
	if (fb_data)
		munmap(fb_data, fb_params.size);

	drm_rm_fb(fb_id);
	drm_mode_destroy_dumb dd{fb_params.handle};
	drm_destroy_dumb(dd);
}

uint32_t Framebuffer::id() const noexcept {
	return fb_id;
}

uint8_t *Framebuffer::data() const noexcept {
	return static_cast<uint8_t *>(fb_data);
}

const drm_mode_create_dumb &Framebuffer::params() const noexcept {
	return fb_params;
}

// Encoder

void Encoder::drm_get_encoder(drm_mode_get_encoder &__enc) const {
	__enc.encoder_id = enc_id;

	if (ioctl(fd_dri, DRM_IOCTL_MODE_GETENCODER, &__enc))
		THROW_SYSTEM_ERROR("DRM_IOCTL_MODE_GETENCODER");
}

uint32_t Encoder::id() const noexcept {
	return enc_id;
}

uint32_t Encoder::encoder_type() const {
	drm_mode_get_encoder enc{};
	drm_get_encoder(enc);

	return enc.encoder_type;
}

uint32_t Encoder::crtc_id() const {
	drm_mode_get_encoder enc{};
	drm_get_encoder(enc);

	return enc.crtc_id;
}

// Connector

void Connector::drm_get_connector(drm_mode_get_connector &__conn) const {
	__conn.connector_id = conn_id;

	if (ioctl(fd_dri, DRM_IOCTL_MODE_GETCONNECTOR, &__conn))
		THROW_SYSTEM_ERROR("DRM_IOCTL_MODE_GETCONNECTOR");
}

uint32_t Connector::id() const noexcept {
	return conn_id;
}

std::vector<drm_mode_modeinfo> Connector::modes() const {
	drm_mode_get_connector conn{};

	drm_get_connector(conn);
	conn.count_props = conn.count_encoders = 0;
	std::vector<drm_mode_modeinfo> mode_ids(conn.count_modes);
	conn.modes_ptr = reinterpret_cast<__u64>(mode_ids.data());
	drm_get_connector(conn);

	return mode_ids;
}

uint32_t Connector::connection() const {
	drm_mode_get_connector conn{};
	drm_get_connector(conn);
	return conn.connection;
}

// Device

void Device::drm_get_resources(drm_mode_card_res &__res) const {
	if (ioctl(fd_dri, DRM_IOCTL_MODE_GETRESOURCES, &__res))
		THROW_SYSTEM_ERROR("DRM_IOCTL_MODE_GETRESOURCES");
}

void Device::drm_get_crtc(drm_mode_crtc &__crtc) const {
	if (ioctl(fd_dri, DRM_IOCTL_MODE_GETCRTC, &__crtc))
		THROW_SYSTEM_ERROR("DRM_IOCTL_MODE_GETCRTC");
}

void Device::drm_set_crtc(const drm_mode_crtc &__crtc) const {
	if (ioctl(fd_dri, DRM_IOCTL_MODE_SETCRTC, &__crtc))
		THROW_SYSTEM_ERROR("DRM_IOCTL_MODE_SETCRTC");
}

Device::Device(const std::string &__path) {
	fd_dri = open(__path.c_str(), O_RDWR);

	if (fd_dri < 0) {
		THROW_SYSTEM_ERROR("failed to open dri device");
	}

	path_ = __path;
}

Device::~Device() {
	if (fd_dri > 0)
		close(fd_dri);
}

void Device::set_master(bool __enabled) {
	if (__enabled) {
		if (ioctl(fd_dri, DRM_IOCTL_SET_MASTER, 0))
			THROW_SYSTEM_ERROR("DRM_IOCTL_SET_MASTER");
	} else {
		if (ioctl(fd_dri, DRM_IOCTL_DROP_MASTER, 0))
			THROW_SYSTEM_ERROR("DRM_IOCTL_DROP_MASTER");
	}
}

std::vector<Connector> Device::connectors() const {
	drm_mode_card_res res{};

	drm_get_resources(res);
	std::vector<uint64_t> conn_ids(res.count_connectors);
	res.connector_id_ptr = reinterpret_cast<uint64_t>(conn_ids.data());
	res.count_fbs = res.count_crtcs = res.count_encoders = 0;
	drm_get_resources(res);

	std::vector<Connector> ret;
	ret.reserve(conn_ids.size());

	for (auto it : conn_ids) {
		ret.emplace_back(fd_dri, it);
	}

	return ret;
}

std::vector<Encoder> Device::encoders() const {
	drm_mode_card_res res{};

	drm_get_resources(res);
	std::vector<uint64_t> enc_ids(res.count_encoders);
	res.encoder_id_ptr = reinterpret_cast<uint64_t>(enc_ids.data());
	res.count_fbs = res.count_crtcs = res.count_connectors = 0;
	drm_get_resources(res);

	std::vector<Encoder> ret;
	ret.reserve(enc_ids.size());

	for (auto it : enc_ids) {
		ret.emplace_back(fd_dri, it);
	}

	return ret;
}

drm_mode_crtc Device::get_crtc(uint32_t __crtc_id) const {
	drm_mode_crtc crtc{};
	crtc.crtc_id = __crtc_id;
	drm_get_crtc(crtc);
	return crtc;
}

void Device::set_crtc(const drm_mode_crtc &__crtc) {
	drm_set_crtc(__crtc);
}

void Device::set_crtc(uint32_t __crtc_id, uint32_t __fb_id, uint32_t __x, uint32_t __y, uint32_t *__connector_ids,
		      uint32_t __connector_count) {
	drm_mode_crtc crtc{};
	crtc.x = __x;
	crtc.y = __y;
	crtc.crtc_id = __crtc_id;
	crtc.fb_id = __fb_id;
	crtc.set_connectors_ptr = reinterpret_cast<__u64>(__connector_ids);
	crtc.count_connectors = __connector_count;
	drm_set_crtc(crtc);
}

void Device::set_crtc(uint32_t __crtc_id, uint32_t __fb_id, uint32_t __x, uint32_t __y, uint32_t *__connector_ids,
		      uint32_t __connector_count, const drm_mode_modeinfo &__mode) {
	drm_mode_crtc crtc{};
	crtc.x = __x;
	crtc.y = __y;
	crtc.crtc_id = __crtc_id;
	crtc.fb_id = __fb_id;
	crtc.set_connectors_ptr = (uint64_t)(__connector_ids);
	crtc.count_connectors = __connector_count;
	crtc.mode = __mode;
	crtc.mode_valid = 1;
	drm_set_crtc(crtc);
}

void Device::set_crtc(const drm_mode_crtc &__crtc, const Connector &__connector) {
	auto c = __crtc;
	auto cid = __connector.id();
	c.set_connectors_ptr = reinterpret_cast<__u64>(&cid);
	c.count_connectors = 1;
	drm_set_crtc(c);
}

void Device::set_crtc(const drm_mode_crtc &__crtc, const Connector &__connector, const drm_mode_modeinfo &__mode) {
	auto c = __crtc;
	auto cid = __connector.id();
	c.set_connectors_ptr = reinterpret_cast<__u64>(&cid);
	c.count_connectors = 1;
	c.mode = __mode;
	c.mode_valid = 1;
	drm_set_crtc(c);
}

void Device::set_crtc(uint32_t __crtc_id, const Framebuffer &__frame_buffer, const Connector &__connector) {
	auto cid = __connector.id();
	auto modes = __connector.modes();

	if (modes.empty())
		throw std::range_error("no modes available for connector");

	set_crtc(__crtc_id, __frame_buffer.id(), 0, 0, &cid, 1, modes[0]);
}

Framebuffer Device::create_framebuffer(uint32_t __width, uint32_t __height, uint8_t __depth, uint8_t __bpp) {
	drm_mode_create_dumb cd{};
	cd.width = __width;
	cd.height = __height;
	cd.bpp = __bpp;

	return {fd_dri, __depth, cd};
}
