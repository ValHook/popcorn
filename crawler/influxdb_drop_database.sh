#!/bin/bash
query="DROP DATABASE allocine;"
echo $query | influx
