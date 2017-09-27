#!/bin/bash
./influxdb_drop_database.sh
./influxdb_create_database.sh
rm -rf dump
SPARK_HOME=/home/ouphi/spark-2.0.2-bin-hadoop2.7
$SPARK_HOME/bin/spark-submit --master local[128] parser.py
