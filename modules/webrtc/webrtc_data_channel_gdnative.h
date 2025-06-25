/**************************************************************************/
/*  webrtc_data_channel_gdnative.h                                        */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#ifndef WEBRTC_DATA_CHANNEL_GDNATIVE_H
#define WEBRTC_DATA_CHANNEL_GDNATIVE_H

#ifdef WEBRTC_GDNATIVE_ENABLED

#include "modules/gdnative/include/net/godot_net.h"
#include "webrtc_data_channel.h"

class WebRTCDataChannelGDNative : public WebRTCDataChannel {
	GDCLASS(WebRTCDataChannelGDNative, WebRTCDataChannel);

protected:
	static void _bind_methods();

private:
	const godot_net_webrtc_data_channel *interface;

public:
	void set_native_webrtc_data_channel(const godot_net_webrtc_data_channel *p_impl);

	void set_write_mode(WriteMode mode) override;
	WriteMode get_write_mode() const override;
	bool was_string_packet() const override;

	ChannelState get_ready_state() const override;
	String get_label() const override;
	bool is_ordered() const override;
	int get_id() const override;
	int get_max_packet_life_time() const override;
	int get_max_retransmits() const override;
	String get_protocol() const override;
	bool is_negotiated() const override;
	int get_buffered_amount() const override;

	Error poll() override;
	void close() override;

	/** Inherited from PacketPeer: **/
	int get_available_packet_count() const override;
	Error get_packet(const uint8_t **r_buffer, int &r_buffer_size) override; ///< buffer is GONE after next get_packet
	Error put_packet(const uint8_t *p_buffer, int p_buffer_size) override;

	int get_max_packet_size() const override;

	WebRTCDataChannelGDNative();
	~WebRTCDataChannelGDNative() override;
};

#endif // WEBRTC_GDNATIVE_ENABLED

#endif // WEBRTC_DATA_CHANNEL_GDNATIVE_H
