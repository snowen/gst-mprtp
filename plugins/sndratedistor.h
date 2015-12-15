/*
 * sndrate_distor.h
 *
 *  Created on: Jun 30, 2015
 *      Author: balazs
 */

#ifndef SNDRATEDISTOR_H_
#define SNDRATEDISTOR_H_

#include <gst/gst.h>
#include "mprtpspath.h"

typedef struct _SendingRateDistributor SendingRateDistributor;
typedef struct _SendingRateDistributorClass SendingRateDistributorClass;
#define SNDRATEDISTOR_TYPE             (sndrate_distor_get_type())
#define SNDRATEDISTOR(src)             (G_TYPE_CHECK_INSTANCE_CAST((src),SNDRATEDISTOR_TYPE,SendingRateDistributor))
#define SNDRATEDISTOR_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),SNDRATEDISTOR_TYPE,SendingRateDistributorClass))
#define SNDRATEDISTOR_IS_SOURCE(src)          (G_TYPE_CHECK_INSTANCE_TYPE((src),SNDRATEDISTOR_TYPE))
#define SNDRATEDISTOR_IS_SOURCE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),SNDRATEDISTOR_TYPE))
#define SNDRATEDISTOR_CAST(src)        ((SendingRateDistributor *)(src))

#define SNDRATEDISTOR_MAX_NUM 32
typedef void  (*SignalRequestFunc)(gpointer,gpointer);

struct _SendingRateDistributor
{
  GObject              object;
  GstClock*            sysclock;
  guint8*              subflows;
  guint8               max_id;
  GQueue*              free_ids;
  guint8               controlled_num;

//  gint32              taken_bytes;
  gint32              supplied_bytes;
  gint32              requested_bytes;
//  gint32              fallen_bytes;
//  gint32              overused_bytes;

  guint32             target_media_rate;
  guint32             actual_media_rate;

  guint8              available_ids[SNDRATEDISTOR_MAX_NUM];
  guint8              available_ids_length;

  guint               monitored;
  guint               overused;

  SignalRequestFunc    signal_request;
  gpointer             signal_controller;
};


struct _SendingRateDistributorClass{
  GObjectClass parent_class;
};


GType sndrate_distor_get_type (void);
SendingRateDistributor *make_sndrate_distor(SignalRequestFunc signal_request, gpointer controller);
guint8 sndrate_distor_request_id(SendingRateDistributor *this,
                                 MPRTPSPath *path,
                                 guint32 sending_rate);
void sndrate_distor_measurement_update(SendingRateDistributor *this,
                                       guint8 id,
                                       RRMeasurement *measurement);

void sndrate_distor_remove_id(SendingRateDistributor *this, guint8 id);
void sndrate_distor_time_update(SendingRateDistributor *this);
gdouble sndrate_distor_get_sending_weight(SendingRateDistributor *this, guint8 id);
gboolean sndrate_distor_congestion_event(SendingRateDistributor *this, guint8 id);
gboolean sndrate_distor_settlemen_event(SendingRateDistributor *this, guint8 id);
#endif /* SNDRATEDISTOR_H_ */
