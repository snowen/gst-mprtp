/*
 * refctrler.h
 *
 *  Created on: Jun 30, 2015
 *      Author: balazs
 */

#ifndef MPRTP_LOGGER_H_
#define MPRTP_LOGGER_H_

#include <gst/gst.h>

typedef struct _MPRTPLogger MPRTPLogger;
typedef struct _MPRTPLoggerClass MPRTPLoggerClass;

#define MPRTPLOGGER_TYPE             (mprtp_logger_get_type())
#define MPRTPLOGGER(src)             (G_TYPE_CHECK_INSTANCE_CAST((src),MPRTPLOGGER_TYPE,MPRTPLogger))
#define MPRTPLOGGER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MPRTPLOGGER_TYPE,MPRTPLoggerClass))
#define MPRTPLOGGER_IS_SOURCE(src)          (G_TYPE_CHECK_INSTANCE_TYPE((src),MPRTPLOGGER_TYPE))
#define MPRTPLOGGER_IS_SOURCE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MPRTPLOGGER_TYPE))
#define MPRTPLOGGER_CAST(src)        ((MPRTPLogger *)(src))


struct _MPRTPLogger
{
  GObject           object;
  GRWLock           rwmutex;
  GstClock*         sysclock;
  GHashTable*       touches;
  gboolean          enabled;
};

struct _MPRTPLoggerClass{
  GObjectClass parent_class;
};

void init_mprtp_logger(void);
void enable_mprtp_logger(void);
void disable_mprtp_logger(void);
void mprtp_logger(const gchar *filename, const gchar * format, ...);

GType mprtp_logger_get_type (void);
#endif /* MPRTP_LOGGER_H_ */