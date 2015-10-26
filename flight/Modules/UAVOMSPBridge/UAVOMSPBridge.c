/**
 ******************************************************************************
 * @addtogroup TauLabsModules TauLabs Modules
 * @{
 * @addtogroup UAVOMSPBridge UAVO to MSP Bridge Module
 * @{
 *
 * @file       uavomspbridge.c
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2015
 * @brief      Bridges selected UAVObjects to MSP for MWOSD and the like.
 *
 * @see        The GNU Public License (GPL) Version 3
 *
 *****************************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "openpilot.h"
#include "physical_constants.h"
#include "modulesettings.h"
#include "flightbatterysettings.h"
#include "flightbatterystate.h"
#include "gpsposition.h"
#include "manualcontrolcommand.h"
#include "attitudeactual.h"
#include "airspeedactual.h"
#include "actuatorsettings.h"
#include "actuatordesired.h"
#include "flightstatus.h"
#include "systemstats.h"
#include "homelocation.h"
#include "baroaltitude.h"
#include "pios_thread.h"
#include "pios_sensors.h"

#include "baroaltitude.h"
#include "flightbatterysettings.h"
#include "flightbatterystate.h"
#include "gpsposition.h"
#include "modulesettings.h"

#if defined(PIOS_INCLUDE_MSP_BRIDGE)

#define MSP_SENSOR_ACC 1
#define MSP_SENSOR_BARO 2
#define MSP_SENSOR_MAG 4
#define MSP_SENSOR_GPS 8

// Magic numbers copied from mwosd
#define  MSP_IDENT      100 // multitype + multiwii version + protocol version + capability variable
#define  MSP_STATUS     101 // cycletime & errors_count & sensor present & box activation & current setting number
#define  MSP_RAW_IMU    102 // 9 DOF
#define  MSP_SERVO      103 // 8 servos
#define  MSP_MOTOR      104 // 8 motors
#define  MSP_RC         105 // 8 rc chan and more
#define  MSP_RAW_GPS    106 // fix, numsat, lat, lon, alt, speed, ground course
#define  MSP_COMP_GPS   107 // distance home, direction home
#define  MSP_ATTITUDE   108 // 2 angles 1 heading
#define  MSP_ALTITUDE   109 // altitude, variometer
#define  MSP_ANALOG     110 // vbat, powermetersum, rssi if available on RX
#define  MSP_RC_TUNING  111 // rc rate, rc expo, rollpitch rate, yaw rate, dyn throttle PID
#define  MSP_PID        112 // P I D coeff (9 are used currently)
#define  MSP_BOX        113 // BOX setup (number is dependant of your setup)
#define  MSP_MISC       114 // powermeter trig
#define  MSP_MOTOR_PINS 115 // which pins are in use for motors & servos, for GUI
#define  MSP_BOXNAMES   116 // the aux switch names
#define  MSP_PIDNAMES   117 // the PID names
#define  MSP_BOXIDS     119 // get the permanent IDs associated to BOXes
#define  MSP_NAV_STATUS 121 // Returns navigation status
#define  MSP_CELLS      130 // FrSky SPort Telemtry

typedef enum {
	MSP_BOX_ARM,
	MSP_BOX_ANGLE,
	MSP_BOX_HORIZON,
	MSP_BOX_BARO,
	MSP_BOX_VARIO,
	MSP_BOX_MAG,
	MSP_BOX_GPSHOME,
	MSP_BOX_GPSHOLD,
	MSP_BOX_LAST,
} msp_box_t;

static struct {
	msp_box_t mode;
	uint8_t mwboxid;
	FlightStatusFlightModeOptions tlmode;
} msp_boxes[] = {
	{ MSP_BOX_ARM, 0, 0 },
	{ MSP_BOX_ANGLE, 1, FLIGHTSTATUS_FLIGHTMODE_LEVELING},
	{ MSP_BOX_HORIZON, 2, FLIGHTSTATUS_FLIGHTMODE_HORIZON},
	{ MSP_BOX_BARO, 3, FLIGHTSTATUS_FLIGHTMODE_ALTITUDEHOLD},
	{ MSP_BOX_VARIO, 4, 0},
	{ MSP_BOX_MAG, 5, 0},
	{ MSP_BOX_GPSHOME, 10, FLIGHTSTATUS_FLIGHTMODE_RETURNTOHOME},
	{ MSP_BOX_GPSHOLD, 11, FLIGHTSTATUS_FLIGHTMODE_POSITIONHOLD},
	{ MSP_BOX_LAST, 0xff, 0},
};

typedef enum {
	MSP_IDLE,
	MSP_HEADER_START,
	MSP_HEADER_M,
	MSP_HEADER_SIZE,
	MSP_HEADER_CMD,
	MSP_FILLBUF,
	MSP_CHECKSUM,
	MSP_DISCARD,
	MSP_MAYBE_UAVTALK2,
	MSP_MAYBE_UAVTALK3,
	MSP_MAYBE_UAVTALK4,
} msp_state;

struct msp_bridge {
	uintptr_t com;

	msp_state _state;
	uint8_t _cmd_size;
	uint8_t _cmd_id;
	uint8_t _cmd_i;
	uint8_t _checksum;
	union {
		uint8_t data[0];
		// Specific packed data structures go here.
	} _cmd_data;
};

#if defined(PIOS_MSP_STACK_SIZE)
#define STACK_SIZE_BYTES PIOS_MSP_STACK_SIZE
#else
#define STACK_SIZE_BYTES 672
#endif
#define TASK_PRIORITY               PIOS_THREAD_PRIO_LOW

static bool module_enabled;
extern uintptr_t pios_com_msp_id;
static struct msp_bridge *msp;
static int32_t uavoMSPBridgeInitialize(void);
static void uavoMSPBridgeTask(void *parameters);

static void msp_send(struct msp_bridge *m, uint8_t cmd, uint8_t *data, size_t len)
{
	uint8_t buf[5];
	uint8_t cs = (uint8_t)(len) ^ cmd;

	buf[0] = '$';
	buf[1] = 'M';
	buf[2] = '>';
	buf[3] = (uint8_t)(len);
	buf[4] = cmd;

	PIOS_COM_SendBuffer(m->com, buf, sizeof(buf));
	PIOS_COM_SendBuffer(m->com, data, len);

	for (int i = 0; i < len; i++) {
		cs ^= data[i];
	}
	cs ^= 0;

	buf[0] = cs;
	PIOS_COM_SendBuffer(m->com, buf, 1);
}

static msp_state msp_state_size(struct msp_bridge *m, uint8_t b)
{
	m->_cmd_size = b;
	m->_checksum = b;
	return MSP_HEADER_CMD;
}

static msp_state msp_state_cmd(struct msp_bridge *m, uint8_t b)
{
	m->_cmd_i = 0;
	m->_cmd_id = b;
	m->_checksum ^= m->_cmd_id;

	if (m->_cmd_size > sizeof(m->_cmd_data)) {
		// Too large a body.  Let's ignore it.
		return MSP_DISCARD;
	}

	return m->_cmd_size == 0 ? MSP_CHECKSUM : MSP_FILLBUF;
}

static msp_state msp_state_fill_buf(struct msp_bridge *m, uint8_t b)
{
	m->_cmd_data.data[m->_cmd_i++] = b;
	m->_checksum ^= b;
	return m->_cmd_i == m->_cmd_size ? MSP_CHECKSUM : MSP_FILLBUF;
}

static void _msp_send_attitude(struct msp_bridge *m)
{
	union {
		uint8_t buf[0];
		struct {
			int16_t x;
			int16_t y;
			int16_t h;
		} att;
	} data;
	AttitudeActualData attActual;

	AttitudeActualGet(&attActual);

	data.att.x = attActual.Roll * 10;
	data.att.y = attActual.Pitch * -10;
	data.att.h = 0 * 10;

	msp_send(m, MSP_ATTITUDE, data.buf, sizeof(data));
}

static void _msp_send_status(struct msp_bridge *m)
{
	union {
		uint8_t buf[0];
		struct {
			uint16_t cycleTime;
			uint16_t i2cErrors;
			uint16_t sensors;
			uint32_t flags;
			uint8_t setting;
		} __attribute__((packed)) status;
	} data;
	// TODO: https://github.com/TauLabs/TauLabs/blob/next/shared/uavobjectdefinition/actuatordesired.xml#L8
	data.status.cycleTime = 0;
	data.status.i2cErrors = 0;
	data.status.sensors =
		(PIOS_SENSORS_IsRegistered(PIOS_SENSOR_ACCEL) ? MSP_SENSOR_ACC : 0)
		| (PIOS_SENSORS_IsRegistered(PIOS_SENSOR_BARO) ? MSP_SENSOR_BARO : 0)
		| (PIOS_SENSORS_IsRegistered(PIOS_SENSOR_MAG) ? MSP_SENSOR_MAG : 0);
	data.status.flags = 0;
	data.status.setting = 0;

	if (FlightStatusHandle() != NULL) {
		FlightStatusData flight_status;
		FlightStatusGet(&flight_status);

		data.status.flags = flight_status.Armed == FLIGHTSTATUS_ARMED_ARMED;

		for (int i = 1; msp_boxes[i].mode != MSP_BOX_LAST; i++) {
			if (flight_status.FlightMode == msp_boxes[i].tlmode) {
				data.status.flags |= (1 << i);
			}
		}
	}

	msp_send(m, MSP_STATUS, data.buf, sizeof(data));
}

static void _msp_send_analog(struct msp_bridge *m)
{
	union {
		uint8_t buf[0];
		struct {
			uint8_t vbat;
			uint16_t powerMeterSum;
			uint16_t rssi;
			uint16_t current;
		} __attribute__((packed)) status;
	} data;
	data.status.powerMeterSum = 0;

	FlightBatterySettingsData batSettings = {};
	FlightBatteryStateData batState = {};

	if (FlightBatteryStateHandle() != NULL)
		FlightBatteryStateGet(&batState);
	if (FlightBatterySettingsHandle() != NULL) {
		FlightBatterySettingsGet(&batSettings);
	}

	// TODO:  Verify these with a board that has working power stuff
	if (batSettings.VoltagePin != FLIGHTBATTERYSETTINGS_VOLTAGEPIN_NONE)
		data.status.vbat = (uint8_t)lroundf(batState.Voltage * 10);

	if (batSettings.CurrentPin != FLIGHTBATTERYSETTINGS_CURRENTPIN_NONE)
		data.status.current = lroundf(batState.Current * 10);

	ManualControlCommandData manualState;
	ManualControlCommandGet(&manualState);

	// MSP RSSI's range is 0-1023
	data.status.rssi = (manualState.Rssi >= 0 && manualState.Rssi <= 100) ? manualState.Rssi * 10 : 0;

	msp_send(m, MSP_ANALOG, data.buf, sizeof(data));
}

static void _msp_send_ident(struct msp_bridge *m)
{
	// TODO
}

static void _msp_send_raw_gps(struct msp_bridge *m)
{
	// TODO
}

static void _msp_send_comp_gps(struct msp_bridge *m)
{
	// TODO
}

static void _msp_send_altitude(struct msp_bridge *m)
{
	union {
		uint8_t buf[0];
		struct {
			int32_t alt; // cm
			uint16_t vario; // cm/s
		} __attribute__((packed)) baro;
	} data;

	BaroAltitudeData baro;
	if (BaroAltitudeHandle() != NULL)
		BaroAltitudeGet(&baro);

	data.baro.alt = (int32_t)roundf(baro.Altitude * 100.0f);

	msp_send(m, MSP_ALTITUDE, data.buf, sizeof(data));
}

static void _msp_send_channels(struct msp_bridge *m)
{
	ManualControlCommandData manualState;
	ManualControlCommandGet(&manualState);

	// MSP RC order is Roll/Pitch/Yaw/Throttle/AUX1/AUX2/AUX3/AUX4
	static const uint8_t channel_map[] = {
		MANUALCONTROLCOMMAND_CHANNEL_ROLL,
		MANUALCONTROLCOMMAND_CHANNEL_PITCH,
		MANUALCONTROLCOMMAND_CHANNEL_YAW,
		MANUALCONTROLCOMMAND_CHANNEL_THROTTLE,
		MANUALCONTROLCOMMAND_CHANNEL_FLIGHTMODE,
		MANUALCONTROLCOMMAND_CHANNEL_ACCESSORY0,
		MANUALCONTROLCOMMAND_CHANNEL_ACCESSORY1,
		MANUALCONTROLCOMMAND_CHANNEL_ACCESSORY2,
	};

	union {
		uint8_t buf[0];
		uint16_t channels[8];
	} data;

	int throttle = manualState.Channel[MANUALCONTROLCOMMAND_CHANNEL_THROTTLE];
	if (throttle > 500 && throttle < 3000) {
		// Normal stuff.
		for (int i = 0; i < 8; i++) {
			data.channels[i] = manualState.Channel[channel_map[i]];
		}
	} else {
		// Out of bound values indicate rc is not connected.
		ActuatorSettingsData actuatorSettings;
		ActuatorSettingsGet(&actuatorSettings);
		for (int i = 0; i < 8; i++) {
			data.channels[i] = actuatorSettings.ChannelNeutral[channel_map[i]];
		}
	}

	msp_send(m, MSP_RC, data.buf, sizeof(data));
}

static void _msp_send_boxids(struct msp_bridge *m) {
	uint8_t boxes[MSP_BOX_LAST];
	int len = 0;

	for (int i = 0; msp_boxes[i].mode != MSP_BOX_LAST; i++) {
		boxes[len++] = msp_boxes[i].mwboxid;
	}
	msp_send(m, MSP_BOXIDS, boxes, len);
}

static msp_state msp_state_checksum(struct msp_bridge *m, uint8_t b)
{
	if ((m->_checksum ^ b) != 0) {
		return MSP_IDLE;
	}

	// Respond to interesting things.
	switch (m->_cmd_id) {
	case MSP_IDENT:
		_msp_send_ident(m);
		break;
	case MSP_RAW_GPS:
		_msp_send_raw_gps(m);
		break;
	case MSP_COMP_GPS:
		_msp_send_comp_gps(m);
		break;
	case MSP_ALTITUDE:
		_msp_send_altitude(m);
		break;
	case MSP_ATTITUDE:
		_msp_send_attitude(m);
		break;
	case MSP_STATUS:
		_msp_send_status(m);
		break;
	case MSP_ANALOG:
		_msp_send_analog(m);
		break;
	case MSP_RC:
		_msp_send_channels(m);
		break;
	case MSP_BOXIDS:
		_msp_send_boxids(m);
		break;
	}
	return MSP_IDLE;
}

static msp_state msp_state_discard(struct msp_bridge *m, uint8_t b)
{
	return m->_cmd_i++ == m->_cmd_size ? MSP_IDLE : MSP_DISCARD;
}

/**
 * Process incoming bytes from an MSP query thing.
 * @param[in] b received byte
 * @return true if we should continue processing bytes
 */
static bool msp_receive_byte(struct msp_bridge *m, uint8_t b)
{
	switch (m->_state) {
	case MSP_IDLE:
		switch (b) {
		case '<': // uavtalk matching with 0x3c 0x2x 0xxx 0x0x
			m->_state = MSP_MAYBE_UAVTALK2;
			break;
		case '$':
			m->_state = MSP_HEADER_START;
			break;
		default:
			m->_state = MSP_IDLE;
		}
		break;
	case MSP_HEADER_START:
		m->_state = b == 'M' ? MSP_HEADER_M : MSP_IDLE;
		break;
	case MSP_HEADER_M:
		m->_state = b == '<' ? MSP_HEADER_SIZE : MSP_IDLE;
		break;
	case MSP_HEADER_SIZE:
		m->_state = msp_state_size(m, b);
		break;
	case MSP_HEADER_CMD:
		m->_state = msp_state_cmd(m, b);
		break;
	case MSP_FILLBUF:
		m->_state = msp_state_fill_buf(m, b);
		break;
	case MSP_CHECKSUM:
		m->_state = msp_state_checksum(m, b);
		break;
	case MSP_DISCARD:
		m->_state = msp_state_discard(m, b);
		break;
	case MSP_MAYBE_UAVTALK2:
		// e.g. 3c 20 1d 00
		// second possible uavtalk byte
		m->_state = (b&0xf0) == 0x20 ? MSP_MAYBE_UAVTALK3 : MSP_IDLE;
		break;
	case MSP_MAYBE_UAVTALK3:
		// third possible uavtalk byte can be anything
		m->_state = MSP_MAYBE_UAVTALK4;
		break;
	case MSP_MAYBE_UAVTALK4:
		m->_state = MSP_IDLE;
		// If this looks like the fourth possible uavtalk byte, we're done
		if ((b & 0xf0) == 0) {
			PIOS_COM_TELEM_RF = m->com;
			return false;
		}
		break;
	}

	return true;
}

/**
 * Module start routine automatically called after initialization routine
 * @return 0 when was successful
 */
static int32_t uavoMSPBridgeStart(void)
{
	if (!module_enabled)
		return -1;

	struct pios_thread *task = PIOS_Thread_Create(
		uavoMSPBridgeTask, "uavoMSPBridge",
		STACK_SIZE_BYTES, NULL, TASK_PRIORITY);
	TaskMonitorAdd(TASKINFO_RUNNING_UAVOMSPBRIDGE,
			task);

	return 0;
}

static void setMSPSpeed(struct msp_bridge *m)
{
	if (m->com) {
		uint8_t speed;
		ModuleSettingsMSPSpeedGet(&speed);

		switch (speed) {
		case MODULESETTINGS_MSPSPEED_2400:
			PIOS_COM_ChangeBaud(m->com, 2400);
			break;
		case MODULESETTINGS_MSPSPEED_4800:
			PIOS_COM_ChangeBaud(m->com, 4800);
			break;
		case MODULESETTINGS_MSPSPEED_9600:
			PIOS_COM_ChangeBaud(m->com, 9600);
			break;
		case MODULESETTINGS_MSPSPEED_19200:
			PIOS_COM_ChangeBaud(m->com, 19200);
			break;
		case MODULESETTINGS_MSPSPEED_38400:
			PIOS_COM_ChangeBaud(m->com, 38400);
			break;
		case MODULESETTINGS_MSPSPEED_57600:
			PIOS_COM_ChangeBaud(m->com, 57600);
			break;
		case MODULESETTINGS_MSPSPEED_115200:
			PIOS_COM_ChangeBaud(m->com, 115200);
			break;
		}
	}
}


/**
 * Module initialization routine
 * @return 0 when initialization was successful
 */
static int32_t uavoMSPBridgeInitialize(void)
{
	uint8_t module_state[MODULESETTINGS_ADMINSTATE_NUMELEM];
	ModuleSettingsAdminStateGet(module_state);

	if (pios_com_msp_id && (module_state[MODULESETTINGS_ADMINSTATE_UAVOMSPBRIDGE]
			== MODULESETTINGS_ADMINSTATE_ENABLED)) {

		msp = PIOS_malloc(sizeof(*msp));
		if (msp != NULL) {
			memset(msp, 0x00, sizeof(*msp));

			msp->com = pios_com_msp_id;

			setMSPSpeed(msp);
			module_enabled = true;
			return 0;
		}
	}

	module_enabled = false;

	return -1;
}
MODULE_INITCALL(uavoMSPBridgeInitialize, uavoMSPBridgeStart)

/**
 * Main task routine
 * @param[in] parameters parameter given by PIOS_Thread_Create()
 */
static void uavoMSPBridgeTask(void *parameters)
{
	while (1) {
		uint8_t b = 0;
		uint16_t count = PIOS_COM_ReceiveBuffer(msp->com, &b, 1, PIOS_QUEUE_TIMEOUT_MAX);
		if (count) {
			if (!msp_receive_byte(msp, b)) {
				// Returning is considered risky here as
				// that's unusual and this is an edge case.
				while (1) {
					PIOS_Thread_Sleep(60*1000);
				}
			}
		}
	}
}

#endif //PIOS_INCLUDE_MSP_BRIDGE
/**
 * @}
 * @}
 */
