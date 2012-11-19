/**
 * \file src/common/format.h
 * \brief glc types and structures
 * \author Sebastian Wick <wick.sebastian@gmail.com>
 * \author Pyry Haulos <pyry.haulos@gmail.com>
 * \date 2007-2012
 */

/**
 * \defgroup format glc format
 *  \{
 */

#ifndef GLC2_FORMAT_H
#define GLC2_FORMAT_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** stream version */
#define GLC_STREAM_VERSION                  0x5
/** file signature = "GLC" */
#define GLC_SIGNATURE                0x00434c47

/** unsigned time in microseconds */
typedef u_int64_t glc_utime_t;
/** signed time in microseconds */
typedef int64_t glc_stime_t;

/** stream identifier type */
typedef int32_t glc_stream_id_t;
/** size, used in stream to ensure compability */
typedef u_int64_t glc_size_t;
/** node identifier type */
typedef int32_t glc_node_id_t;
typedef int32_t glc_shmid_t;

/** flags */
typedef u_int32_t glc_flags_t;

/** callback request function prototype */
typedef void (*callback_request_func_t)(void *arg);

/**
 * \brief stream info structure
 *
 * Each glc stream file should start with stream info
 * structure. [name_size + date_size] sized data area should
 * follow stream info:
 *
 * First [name_size] bytes contain null-terminated application
 * path string. [date_size] bytes starting at [name_size]
 * contain null-terminated date string in UTC format.
 */
typedef struct {
	/** file signature */
	u_int32_t signature;
	/** stream version */
	u_int32_t version;
	/** fps */
	double fps;
	/** flags */
	glc_flags_t flags;
	/** captured program pid */
	u_int32_t pid;
	/** size of captured program's name */
	u_int32_t name_size;
	/** size of date */
	u_int32_t date_size;
	/** reserved */
	u_int64_t reserved1;
	/** reserved */
	u_int64_t reserved2;
} __attribute__((packed)) glc_stream_info_t;

/** stream message type */
typedef u_int8_t glc_message_type_t;
/** end of stream */
#define GLC_MESSAGE_CLOSE              0x01
/** video data message */
#define GLC_MESSAGE_VIDEO_FRAME        0x02
/** video format message */
#define GLC_MESSAGE_VIDEO_FORMAT       0x03
/** lzo-compressed packet */
#define GLC_MESSAGE_LZO                0x04
/** audio format message */
#define GLC_MESSAGE_AUDIO_FORMAT       0x05
/** audio data message */
#define GLC_MESSAGE_AUDIO_DATA         0x06
/** quicklz-compressed packet */
#define GLC_MESSAGE_QUICKLZ            0x07
/** color correction information */
#define GLC_MESSAGE_COLOR              0x08
/** plain container */
#define GLC_MESSAGE_CONTAINER          0x09
/** lzjb-compressed packet */
#define GLC_MESSAGE_LZJB               0x0a
/** callback request */
#define GLC_CALLBACK_REQUEST           0x0b
/** log message */
#define GLC_MESSAGE_LOG                0x0c
/** exit */
#define GLC_MESSAGE_EXIT               0x0d
/** network */
#define GLC_MESSAGE_NETWORK            0x0e
/** network */
#define GLC_MESSAGE_CONNECT            0x0f

/**
 * \brief stream message header
 * \note the size of the message is stored directly before the message header with the type glc_size_t
 */
typedef struct {
	/** stream message type */
	glc_message_type_t type;
} __attribute__((packed)) glc_message_header_t;

/**
 * \brief lzo-compressed message header
 */
typedef struct {
	/** uncompressed data size */
	glc_size_t size;
	/** original message header */
	glc_message_header_t header;
} __attribute__((packed)) glc_lzo_header_t;

/**
 * \brief quicklz-compressed message header
 */
typedef struct {
	/** uncompressed data size */
	glc_size_t size;
	/** original message header */
	glc_message_header_t header;
} __attribute__((packed)) glc_quicklz_header_t;

/**
 * \brief lzjb-compressed message header
 */
typedef struct {
	/** uncompressed data size */
	glc_size_t size;
	/** original message header */
	glc_message_header_t header;
} __attribute__((packed)) glc_lzjb_header_t;

/** video format type */
typedef u_int8_t glc_video_format_t;
/** 24bit BGR, last row first */
#define GLC_VIDEO_BGR                   0x1
/** 32bit BGRA, last row first */
#define GLC_VIDEO_BGRA                  0x2
/** planar YV12 420jpeg */
#define GLC_VIDEO_YCBCR_420JPEG         0x3
/** 24bit RGB, last row first */
#define GLC_VIDEO_RGB                   0x4

/**
 * \brief video format message
 */
typedef struct {
	/** identifier */
	glc_stream_id_t id;
	/** flags */
	glc_flags_t flags;
	/** width */
	u_int32_t width;
	/** height */
	u_int32_t height;
	/** format */
	glc_video_format_t format;
} __attribute__((packed)) glc_video_format_message_t;

/** double-word aligned rows (GL_PACK_ALIGNMENT = 8) */
#define GLC_VIDEO_DWORD_ALIGNED         0x1

/**
 * \brief video data header
 */
typedef struct {
	/** stream identifier */
	glc_stream_id_t id;
	/** time */
	glc_utime_t time;
} __attribute__((packed)) glc_video_frame_header_t;

/** audio format type */
typedef u_int8_t glc_audio_format_t;
/** signed 16bit little-endian */
#define GLC_AUDIO_S16_LE                0x1
/** signed 24bit little-endian */
#define GLC_AUDIO_S24_LE                0x2
/** signed 32bit little-endian */
#define GLC_AUDIO_S32_LE                0x3

/**
 * \brief audio format message
 */
typedef struct {
	/** identifier */
	glc_stream_id_t id;
	/** flags */
	glc_flags_t flags;
	/** rate in Hz */
	u_int32_t rate;
	/** number of channels */
	u_int32_t channels;
	/** format */
	glc_audio_format_t format;
} __attribute__((packed)) glc_audio_format_message_t;

/** interleaved */
#define GLC_AUDIO_INTERLEAVED           0x1

/**
 * \brief audio data message header
 */
typedef struct {
	/** stream identifier */
	glc_stream_id_t id;
	/** time */
	glc_utime_t time;
	/** data size in bytes */
	glc_size_t size;
} __attribute__((packed)) glc_audio_data_header_t;

/**
 * \brief color correction information message
 */
typedef struct {
	/** video stream identifier */
	glc_stream_id_t id;
	/** brightness */
	float brightness;
	/** contrast */
	float contrast;
	/** red gamma */
	float red;
	/** green gamma */
	float green;
	/** blue gamma */
	float blue;
} __attribute__((packed)) glc_color_message_t;

/**
 * \brief container message header
 */
typedef struct {
	/** size */
	glc_size_t size;
	/** header */
	glc_message_header_t header;
} __attribute__((packed)) glc_container_message_header_t;

/**
 * \brief callback request
 * \note only for program internal use (not in on-disk stream)
 * \note may change without stream version bump
 * This message doesn't specify callback address but only data
 * pointer. Callbacks are set per-module basis. Useful for
 * synchronizing.
 */
typedef struct {
	/** pointer to data */
	void *arg;
} glc_callback_request_t;

/** log level */
typedef u_int8_t glc_log_level_t;
/** error */
#define GLC_ERROR 0
/** warning */
#define GLC_WARNING 1
/** performance information */
#define GLC_PERFORMANCE 2
/** information */
#define GLC_INFORMATION 3
/** debug */
#define GLC_DEBUG 4

/** 
 * \brief log message header
 */
typedef struct {
    /** time */
    glc_utime_t time;
    /** log level */
    glc_log_level_t level;
    /** size of the module string */
    glc_size_t module_size;
    /** size of the log string */
    glc_size_t log_size;
} __attribute__((packed)) glc_log_header_t;

/**
 * \brief network message header
 */
typedef struct {
    /** node id (-1:unknown, 0:server, n>0:clients) */
    glc_node_id_t node;
    /** payload header */
    glc_message_header_t payload_header;
    /** payload size */
    glc_size_t payload_size;
} __attribute__((packed)) glc_network_header_t;

/**
 * \brief connect message
 *  sending connect message with node == -1 is a connect
 *  request, the server answeres with the same message
 *  and node > 0
 */
typedef struct {
    /** node id (-1:unknown, 0:server, n>0:clients) */
    glc_node_id_t node;
    /** */
    glc_shmid_t shmid;
} __attribute__((packed)) glc_connect_message_t;

#ifdef __cplusplus
}
#endif

#endif

/**  \} */
