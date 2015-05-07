/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef CHIP_STM32_USB_STREAM_H
#define CHIP_STM32_USB_STREAM_H

/* STM32 USB STREAM driver for Chrome EC */

#include "compile_time_macros.h"
#include "consumer.h"
#include "hooks.h"
#include "in_stream.h"
#include "out_stream.h"
#include "producer.h"
#include "queue.h"
#include "usb.h"

#include <stdint.h>

/*
 * Per-USB stream state stored in RAM.  Zero initialization of this structure
 * by the BSS initialization leaves it in a valid and correctly initialized
 * state, so there is no need currently for a usb_stream_init style function.
 */
struct usb_stream_state {
	/*
	 * Flag indicating that there is a full RX buffer in the USB packet RAM
	 * that we were not able to move into the RX queue because there was
	 * not enough room when the packet was initially received.  The
	 * producer read operation checks this flag so that once there is
	 * room in the queue it can copy the RX buffer into the queue and
	 * restart USB reception by marking the RX buffer as VALID.
	 */
	int rx_waiting;
};

/*
 * Compile time Per-USB stream configuration stored in flash.  Instances of this
 * structure are provided by the user of the USB stream.  This structure binds
 * together all information required to operate a USB stream.
 */
struct usb_stream_config {
	/*
	 * Pointer to usb_stream_state structure.  The state structure
	 * maintains per USB stream information.
	 */
	struct usb_stream_state volatile *state;

	/*
	 * Endpoint index, and pointers to the USB packet RAM buffers.
	 */
	int endpoint;

	/*
	 * Deferred function to call to handle USB and Queue request.
	 */
	void (*deferred)(void);

	size_t rx_size;
	size_t tx_size;

	usb_uint *rx_ram;
	usb_uint *tx_ram;

	struct consumer consumer;
	struct producer producer;
};

/*
 * These function tables are defined by the USB Stream driver and are used to
 * initialize the consumer and producer in the usb_stream_config.
 */
extern struct consumer_ops const usb_stream_consumer_ops;
extern struct producer_ops const usb_stream_producer_ops;

/*
 * Convenience macro for defining USB streams and their associated state and
 * buffers.
 *
 * NAME is used to construct the names of the packet RAM buffers, trampoline
 * functions, usb_stream_state struct, and usb_stream_config struct, the
 * latter is just called NAME.
 *
 * INTERFACE is the index of the USB interface to associate with this
 * stream.
 *
 * INTERFACE_NAME is the index of the USB string descriptor (iInterface).
 *
 * ENDPOINT is the index of the USB bulk endpoint used for receiving and
 * transmitting bytes.
 *
 * RX_SIZE and TX_SIZE are the number of bytes of USB packet RAM to allocate
 * for the RX and TX packets respectively.  The valid values for these
 * parameters are dictated by the USB peripheral.
 *
 * RX_QUEUE and TX_QUEUE are the names of the RX and TX queues that this driver
 * should write to and read from respectively.  They must match the queues
 * that the CONSUMER and PRODUCER read from and write to respectively.
 *
 * CONSUMER and PRODUCER are the names of the consumer and producer objects at
 * the other ends of the RX and TX queues respectively.
 */
/*
 * The following assertions can not be made because they require access to
 * non-const fields, but should be kept in mind.
 *
 * BUILD_ASSERT(RX_QUEUE.buffer_units >= RX_SIZE);
 * BUILD_ASSERT(TX_QUEUE.buffer_units >= TX_SIZE);
 * BUILD_ASSERT(RX_QUEUE.unit_bytes == 1);
 * BUILD_ASSERT(TX_QUEUE.unit_bytes == 1);
 * BUILD_ASSERT(PRODUCER.queue == &TX_QUEUE);
 * BUILD_ASSERT(CONSUMER.queue == &RX_QUEUE);
 */
#define USB_STREAM_CONFIG(NAME,						\
			  INTERFACE,					\
			  INTERFACE_NAME,				\
			  ENDPOINT,					\
			  RX_SIZE,					\
			  TX_SIZE,					\
			  RX_QUEUE,					\
			  TX_QUEUE,					\
			  CONSUMER,					\
			  PRODUCER)					\
									\
	BUILD_ASSERT(RX_SIZE <= USB_MAX_PACKET_SIZE);			\
	BUILD_ASSERT(TX_SIZE <= USB_MAX_PACKET_SIZE);			\
	BUILD_ASSERT(RX_SIZE > 0);					\
	BUILD_ASSERT(TX_SIZE > 0);					\
	BUILD_ASSERT((RX_SIZE <   64 && (RX_SIZE & 0x01) == 0) ||	\
		     (RX_SIZE < 1024 && (RX_SIZE & 0x1f) == 0));	\
	BUILD_ASSERT((TX_SIZE <   64 && (TX_SIZE & 0x01) == 0) ||	\
		     (TX_SIZE < 1024 && (TX_SIZE & 0x1f) == 0));	\
									\
	static usb_uint CONCAT2(NAME, _ep_rx_buffer)[RX_SIZE / 2] __usb_ram; \
	static usb_uint CONCAT2(NAME, _ep_tx_buffer)[TX_SIZE / 2] __usb_ram; \
	static struct usb_stream_state CONCAT2(NAME, _state);		\
	static void CONCAT2(NAME, _deferred_)(void);			\
	struct usb_stream_config const NAME = {				\
		.state     = &CONCAT2(NAME, _state),			\
		.endpoint  = ENDPOINT,					\
		.deferred  = CONCAT2(NAME, _deferred_),			\
		.rx_size   = RX_SIZE,					\
		.tx_size   = TX_SIZE,					\
		.rx_ram    = CONCAT2(NAME, _ep_rx_buffer),		\
		.tx_ram    = CONCAT2(NAME, _ep_tx_buffer),		\
		.consumer  = {						\
			.producer = &PRODUCER,				\
			.queue    = &TX_QUEUE,				\
			.ops      = &usb_stream_consumer_ops,		\
		},							\
		.producer  = {						\
			.consumer = &CONSUMER,				\
			.queue    = &RX_QUEUE,				\
			.ops      = &usb_stream_producer_ops,		\
		},							\
	};								\
	const struct usb_interface_descriptor				\
	USB_IFACE_DESC(INTERFACE) = {					\
		.bLength            = USB_DT_INTERFACE_SIZE,		\
		.bDescriptorType    = USB_DT_INTERFACE,			\
		.bInterfaceNumber   = INTERFACE,			\
		.bAlternateSetting  = 0,				\
		.bNumEndpoints      = 2,				\
		.bInterfaceClass    = USB_CLASS_VENDOR_SPEC,		\
		.bInterfaceSubClass = USB_SUBCLASS_GOOGLE_SERIAL,	\
		.bInterfaceProtocol = USB_PROTOCOL_GOOGLE_SERIAL,	\
		.iInterface         = INTERFACE_NAME,			\
	};								\
	const struct usb_endpoint_descriptor				\
	USB_EP_DESC(INTERFACE, 0) = {					\
		.bLength          = USB_DT_ENDPOINT_SIZE,		\
		.bDescriptorType  = USB_DT_ENDPOINT,			\
		.bEndpointAddress = 0x80 | ENDPOINT,			\
		.bmAttributes     = 0x02 /* Bulk IN */,			\
		.wMaxPacketSize   = TX_SIZE,				\
		.bInterval        = 10,					\
	};								\
	const struct usb_endpoint_descriptor				\
	USB_EP_DESC(INTERFACE, 1) = {					\
		.bLength          = USB_DT_ENDPOINT_SIZE,		\
		.bDescriptorType  = USB_DT_ENDPOINT,			\
		.bEndpointAddress = ENDPOINT,				\
		.bmAttributes     = 0x02 /* Bulk OUT */,		\
		.wMaxPacketSize   = RX_SIZE,				\
		.bInterval        = 0,					\
	};								\
	static void CONCAT2(NAME, _ep_tx)(void)				\
	{								\
		usb_stream_tx(&NAME);					\
	}								\
	static void CONCAT2(NAME, _ep_rx)(void)				\
	{								\
		usb_stream_rx(&NAME);					\
	}								\
	static void CONCAT2(NAME, _ep_reset)(void)			\
	{								\
		usb_stream_reset(&NAME);				\
	}								\
	USB_DECLARE_EP(ENDPOINT,					\
		       CONCAT2(NAME, _ep_tx),				\
		       CONCAT2(NAME, _ep_rx),				\
		       CONCAT2(NAME, _ep_reset));			\
	static void CONCAT2(NAME, _deferred_)(void)			\
	{ usb_stream_deferred(&NAME); }					\
	DECLARE_DEFERRED(CONCAT2(NAME, _deferred_));

/*
 * Handle USB and Queue request in a deferred callback.
 */
void usb_stream_deferred(struct usb_stream_config const *config);

/*
 * These functions are used by the trampoline functions defined above to
 * connect USB endpoint events with the generic USB stream driver.
 */
void usb_stream_tx(struct usb_stream_config const *config);
void usb_stream_rx(struct usb_stream_config const *config);
void usb_stream_reset(struct usb_stream_config const *config);

#endif /* CHIP_STM32_USB_STREAM_H */
