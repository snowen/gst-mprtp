#!/bin/bash

echo -n " "
echo -n "--path1_tx_rtp_port=5000 "
echo -n "--path1_tx_rtcp_port=5001 "
echo -n "--path1_rx_rtp_port=5002 "
echo -n "--path1_rx_rtcp_port=5003 " 
echo -n "--path2_tx_rtp_port=5004 "  
echo -n "--path2_tx_rtcp_port=5005 "  
echo -n "--path2_rx_rtp_port=5006 "  
echo -n "--path2_rx_rtcp_port=5007 "  
echo -n "--path3_tx_rtp_port=5008 "  
echo -n "--path3_tx_rtcp_port=5009 "  
echo -n "--path3_rx_rtp_port=5010 "  
echo -n "--path3_rx_rtcp_port=5011 "  
echo -n "--rtpbin_tx_rtcp_port=5013 "  
echo -n "--rtpbin_rx_rtcp_port=5015 "
echo -n "--logsdir=logs/ "              
echo -n "--testseq=scripts/mprtp1/test_commands.txt "
echo -n "--yuvsrc_file=foreman_cif.yuv "  
echo -n "--yuvsrc_width=352 "  
echo -n "--yuvsrc_height=288 "  
echo -n "--path_1_rx_ip=10.0.0.1 "  
echo -n "--path_2_rx_ip=10.0.1.1 "  
echo -n "--path_3_rx_ip=10.0.2.1 "  
echo -n "--path_1_tx_ip=10.0.0.2 "  
echo -n "--path_2_tx_ip=10.0.1.2 "  
echo -n "--path_3_tx_ip=10.0.2.2 " 
echo -n "--playout_high_watermark=0 "
echo -n "--playout_low_watermark=0 "
echo -n "--playout_desired_framenum=1 "
echo -n "--playout_spread_factor=1.2 "
echo -n "--logging=1 "  
echo -n "--join_min_th=10 "  
echo -n "--join_max_th=100 "  
echo -n "--join_window_th=60 "
echo -n "--join_betha_factor=1.2 "
echo -n "--owd_th=200 "  
echo -n "--discard_th=20 "  
echo -n "--spike_delay_th=500 "  
echo -n "--spike_var_th=63 "  
echo -n "--obsolation_th=200 "
echo -n "--lost_th=1000 "  
echo -n "--rtcp_interval_type=2 "  
echo -n "--report_timeout=0 "  
echo -n "--controlling_mode=2 "  
echo -n "--sending_target=500000 "  
echo -n "--path1_active=1 "  
echo -n "--path2_active=1 "  
echo -n "--path3_active=0 "  
echo -n "--fec_interval=0 "  
echo -n "--fec_min_window=0 "  
echo -n "--fec_max_window=100 "  
echo -n "--keep_alive_period=0 "  

