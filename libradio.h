/*
 * Copyright (c) 2020-21, Kalopa Robotics Limited.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
 * NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ABSTRACT
 */
#define SI4463_PACKET_LEN			48

#define SI4463_NOP					0x00
#define SI4463_PART_INFO			0x01
#define SI4463_POWER_UP				0x02
#define SI4463_FUNC_INFO			0x10
#define SI4463_SET_PROPERTY			0x11
#define SI4463_GET_PROPERTY			0x12
#define SI4463_GPIO_PIN_CFG			0x13
#define SI4463_FIFO_INFO			0x15
#define SI4463_GET_INT_STATUS		0x20
#define SI4463_REQUEST_DEVICE_STATE	0x33
#define SI4463_CHANGE_STATE			0x34
#define SI4463_OFFLINE_RECAL		0x38
#define SI4463_READ_CMD_BUFF		0x44
#define SI4463_FRR_A_READ			0x50
#define SI4463_FRR_B_READ			0x51
#define SI4463_FRR_C_READ			0x53
#define SI4463_FRR_D_READ			0x57

#define SI4463_IRCAL				0x17
#define SI4463_IRCAL_MANUAL			0x19

#define SI4463_START_TX				0x31
#define SI4463_TX_HOP				0x37
#define SI4463_WRITE_TX_FIFO		0x66

#define SI4463_PACKET_INFO			0x16
#define SI4463_GET_MODEM_STATUS		0x22
#define SI4463_START_RX				0x32
#define SI4463_RX_HOP				0x36
#define SI4463_READ_RX_FIFO			0x77

#define SI4463_GET_ADC_READING		0x14
#define SI4463_GET_PH_STATUS		0x21
#define SI4463_GET_CHIP_STATUS		0x23

#define SI4463_STATE_NOCHANGE		0
#define SI4463_STATE_SLEEP			1
#define SI4463_STATE_SPI_ACTIVE		2
#define SI4463_STATE_READY			3
#define SI4463_STATE_READY2			4
#define SI4463_STATE_TX_TUNE		5
#define SI4463_STATE_RX_TUNE		6
#define SI4463_STATE_TX				7
#define SI4463_STATE_RX				8
