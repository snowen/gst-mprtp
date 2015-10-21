/* GStreamer
 * Copyright (C) 2013 Collabora Ltd.
 *   @author Torrie Fischer <torrie.fischer@collabora.co.uk>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */
#include <gst/gst.h>
#include <gst/rtp/rtp.h>

/*
 *                                           .------------.
 *                                           | rtpbin     |
 *  .-------.    .---------.    .---------.  |            |       .--------------.    .-------------.
 *  |audiots|    |theoraenc|    |theorapay|  |            |       |   mprtp_sch  |    | mprtp_snd   |      .-------.
 *  |      src->sink      src->sink  src->send_rtp_0 send_rtp_0->rtp_sink mprtp_src->mprtp_src      |      |udpsink|
 *  '-------'    '---------'    '---------'  |            |       |              |    |            src_0->sink     |
 *                                           |            |       |   mprtcp_sr_src->mprtcp_sr_sink |      '-------'
 *                                           |            |       '--------------'    |             |      .-------.
 *                                           |            |                          mprtcp_rr_sink |      |udpsink|
 *                                           |            |                           |           src_1->sink      |
 *                               .------.    |            |                           '-------------'      '-------'
 *                               |udpsrc|    |            |       .-------.
 *                               |     src->recv_rtcp_1   |       |udpsink|
 *                               '------'    |       send_rtcp_1->sink    |
 *                                           '------------'       '-------'
 */

typedef struct _SessionData
{
  int ref;
  guint sessionNum;
  GstElement *input;
} SessionData;

static SessionData *
session_ref (SessionData * data)
{
  g_atomic_int_inc (&data->ref);
  return data;
}

static void
session_unref (gpointer data)
{
  SessionData *session = (SessionData *) data;
  if (g_atomic_int_dec_and_test (&session->ref)) {
    g_free (session);
  }
}

static SessionData *
session_new (guint sessionNum)
{
  SessionData *ret = g_new0 (SessionData, 1);
  ret->sessionNum = sessionNum;
  return session_ref (ret);
}

/*
 * Used to generate informative messages during pipeline startup
 */
static void
cb_state (GstBus * bus, GstMessage * message, gpointer data)
{
  GstObject *pipe = GST_OBJECT (data);
  GstState old, new, pending;
  gst_message_parse_state_changed (message, &old, &new, &pending);
  if (message->src == pipe) {
    g_print ("Pipeline %s changed state from %s to %s\n",
        GST_OBJECT_NAME (message->src),
        gst_element_state_get_name (old), gst_element_state_get_name (new));
  }
}

/*
 * Creates a GstGhostPad named "src" on the given bin, pointed at the "src" pad
 * of the given element
 */
static void
setup_ghost (GstElement * src, GstBin * bin)
{
  GstPad *srcPad = gst_element_get_static_pad (src, "src");
  GstPad *binPad = gst_ghost_pad_new ("src", srcPad);
  gst_element_add_pad (GST_ELEMENT (bin), binPad);
}

static SessionData *
make_video_session (guint sessionNum)
{
  GstBin *videoBin = GST_BIN (gst_bin_new (NULL));
  GstElement *videoSrc = gst_element_factory_make ("videotestsrc", NULL);
  GstElement *encoder = gst_element_factory_make ("theoraenc", NULL);
  GstElement *payloader = gst_element_factory_make ("rtptheorapay", NULL);
  GstCaps *videoCaps;
  SessionData *session;
  g_object_set (videoSrc, "is-live", TRUE, "horizontal-speed", 1, NULL);
  g_object_set (payloader, "config-interval", 2, NULL);

  gst_bin_add_many (videoBin, videoSrc, encoder, payloader, NULL);
  videoCaps = gst_caps_new_simple ("video/x-raw",
      "width", G_TYPE_INT, 352,
      "height", G_TYPE_INT, 288, "framerate", GST_TYPE_FRACTION, 15, 1, NULL);
  gst_element_link_filtered (videoSrc, encoder, videoCaps);
  gst_element_link (encoder, payloader);

  setup_ghost (payloader, videoBin);

  session = session_new (sessionNum);
  session->input = GST_ELEMENT (videoBin);

  return session;
}

static GstElement *
request_aux_sender (GstElement * rtpbin, guint sessid, SessionData * session)
{
  GstElement *rtx, *bin;
  GstPad *pad;
  gchar *name;
  GstStructure *pt_map;

  GST_INFO ("creating AUX sender");
  bin = gst_bin_new (NULL);
  rtx = gst_element_factory_make ("rtprtxsend", NULL);
  pt_map = gst_structure_new ("application/x-rtp-pt-map",
      "8", G_TYPE_UINT, 98, "96", G_TYPE_UINT, 99, NULL);
  g_object_set (rtx, "payload-type-map", pt_map, NULL);
  gst_structure_free (pt_map);
  gst_bin_add (GST_BIN (bin), rtx);

  pad = gst_element_get_static_pad (rtx, "src");
  name = g_strdup_printf ("src_%u", sessid);
  gst_element_add_pad (bin, gst_ghost_pad_new (name, pad));
  g_free (name);
  gst_object_unref (pad);

  pad = gst_element_get_static_pad (rtx, "sink");
  name = g_strdup_printf ("sink_%u", sessid);
  gst_element_add_pad (bin, gst_ghost_pad_new (name, pad));
  g_free (name);
  gst_object_unref (pad);

  return bin;
}

typedef struct _Identities
{
  GstElement *mprtpsch;
  GstElement *identity_s1;
  GstElement *identity_s2;
  guint called;
} Identities;



static gboolean
_mprtpsch_bidding_test_cb (gpointer data)
{
  Identities *ids = data;

  g_print ("MpRTP Scheduler setup bidding test, called %d\n", ids->called);

  switch (ids->called) {
    case 0:
      g_object_set (ids->mprtpsch,
          "setup-sending-bid", (1 << 24) | 70,
          "setup-sending-bid", (2 << 24) | 30, NULL);
      g_print ("subflow 1: 70%%, subflow 2: 30%%\n");
      break;
    case 3:
      g_object_set (ids->mprtpsch,
          "setup-sending-bid", (1 << 24) | 40,
          "setup-sending-bid", (2 << 24) | 60, NULL);
      g_print ("subflow 1: 40%%, subflow 2: 60%%\n");
      break;
    case 6:
      g_object_set (ids->mprtpsch,
          "setup-sending-bid", (1 << 24) | 9,
          "setup-sending-bid", (2 << 24) | 1, NULL);
      g_print ("subflow 1: 90%%, subflow 2: 10%%\n");
      break;
    case 8:
      g_object_set (ids->mprtpsch,
          "setup-sending-bid", (1 << 24) | 1,
          "setup-sending-bid", (2 << 24) | 99, NULL);
      g_print ("subflow 1: 1%%, subflow 2: 99%%\n");
      break;
    case 10:
      g_object_set (ids->mprtpsch,
          "setup-sending-bid", (1 << 24) | 50,
          "setup-sending-bid", (2 << 24) | 50, NULL);
      g_print ("subflow 1: 50%%, subflow 2: 50%%\n");
      break;
    default:
      break;
  }
  return ++ids->called > 10 ? G_SOURCE_REMOVE : G_SOURCE_CONTINUE;
}



/*
 * This function sets up the UDP sinks and sources for RTP/RTCP, adds the
 * given session's bin into the pipeline, and links it to the properly numbered
 * pads on the rtpbin
 */

static void
add_stream (GstPipeline * pipe, GstElement * rtpBin, SessionData * session)
{

  GstElement *rtpSink_1 = gst_element_factory_make ("udpsink", NULL);
  GstElement *rtpSink_2 = gst_element_factory_make ("udpsink", NULL);
  GstElement *rtcpSink = gst_element_factory_make ("udpsink", NULL);
  GstElement *rtcpSrc = gst_element_factory_make ("udpsrc", NULL);
  GstElement *mprtpsnd = gst_element_factory_make ("mprtpsender", NULL);
  GstElement *mprtpsch = gst_element_factory_make ("mprtpscheduler", NULL);
  GstElement *identity_1 = gst_element_factory_make ("identity", NULL);
  GstElement *identity_2 = gst_element_factory_make ("identity", NULL);
  Identities *ids = g_malloc0 (sizeof (Identities));
  int basePort;
  gchar *padName;

  ids->mprtpsch = mprtpsch;
  ids->identity_s1 = identity_1;
  ids->identity_s2 = identity_2;
  ids->called = 0;

  basePort = 5000 + (session->sessionNum * 20);

  gst_bin_add_many (GST_BIN (pipe), rtpSink_1, rtpSink_2, mprtpsnd,
      mprtpsch, rtcpSink, rtcpSrc, identity_1,
      identity_2, session->input, NULL);

  /* enable retransmission by setting rtprtxsend as the "aux" element of rtpbin */
  g_signal_connect (rtpBin, "request-aux-sender",
      (GCallback) request_aux_sender, session);

  g_object_set (rtpSink_1, "port", basePort, "host", "127.0.0.1",
//      NULL);
      "sync",FALSE, "async", FALSE, NULL);

  g_object_set (rtpSink_2, "port", basePort + 1, "host", "127.0.0.1",
//      NULL);
      "sync",FALSE, "async", FALSE, NULL);

  g_object_set (rtcpSink, "port", basePort + 5, "host", "127.0.0.1",
//      NULL);
       "sync",FALSE, "async", FALSE, NULL);

  g_object_set (rtcpSrc, "port", basePort + 10, NULL);

  g_object_set (mprtpsch, "join-subflow", 1, NULL);
  g_object_set (mprtpsch, "join-subflow", 2, NULL);
  g_object_set (mprtpsch, "auto-flow-controlling", FALSE, NULL);

  g_object_set(ids->identity_s1, "drop-probability", .5, NULL);

  g_timeout_add (10000, _mprtpsch_bidding_test_cb, ids);

  padName = g_strdup_printf ("send_rtp_sink_%u", session->sessionNum);
  gst_element_link_pads (session->input, "src", rtpBin, padName);
  g_free (padName);

  //MPRTP Sender

  padName = g_strdup_printf ("send_rtp_src_%u", session->sessionNum);
  gst_element_link_pads (rtpBin, padName, mprtpsch, "rtp_sink");
  gst_element_link_pads (mprtpsch, "mprtp_src", mprtpsnd, "mprtp_sink");
  g_free (padName);

  gst_element_link_pads (mprtpsnd, "src_1", identity_1, "sink");
  gst_element_link (identity_1, rtpSink_1);

  gst_element_link_pads (mprtpsnd, "src_2", identity_2, "sink");
  gst_element_link (identity_2, rtpSink_2);

  padName = g_strdup_printf ("send_rtcp_src_%u", session->sessionNum);
  gst_element_link_pads (rtpBin, padName, rtcpSink, "sink");
  g_free (padName);


  padName = g_strdup_printf ("recv_rtcp_sink_%u", session->sessionNum);
  gst_element_link_pads (mprtpsch, "mprtcp_sr_src", mprtpsnd, "mprtcp_sr_sink");
  gst_element_link_pads (rtcpSrc, "src", rtpBin, padName);
  g_free (padName);

  g_print ("New RTP stream on %i/%i/%i\n", basePort, basePort + 1,
      basePort + 5);

  session_unref (session);
}

int
main (int argc, char **argv)
{
  GstPipeline *pipe;
  GstBus *bus;
  SessionData *videoSession;
  SessionData *audioSession;
  GstElement *rtpBin;
  GMainLoop *loop;

  gst_init (&argc, &argv);

  loop = g_main_loop_new (NULL, FALSE);

  pipe = GST_PIPELINE (gst_pipeline_new (NULL));
  bus = gst_element_get_bus (GST_ELEMENT (pipe));
  g_signal_connect (bus, "message::state-changed", G_CALLBACK (cb_state), pipe);
  gst_bus_add_signal_watch (bus);
  gst_object_unref (bus);

  rtpBin = gst_element_factory_make ("rtpbin", NULL);
  g_object_set (rtpBin, "rtp-profile", GST_RTP_PROFILE_AVPF, NULL);

  gst_bin_add (GST_BIN (pipe), rtpBin);

  videoSession = make_video_session (0);
  //audioSession = make_audio_session (1);
  add_stream (pipe, rtpBin, videoSession);
  //add_stream (pipe, rtpBin, audioSession);

  g_print ("starting server pipeline\n");
  gst_element_set_state (GST_ELEMENT (pipe), GST_STATE_PLAYING);

  g_main_loop_run (loop);

  g_print ("stopping server pipeline\n");
  gst_element_set_state (GST_ELEMENT (pipe), GST_STATE_NULL);

  gst_object_unref (pipe);
  g_main_loop_unref (loop);

  return 0;
}
