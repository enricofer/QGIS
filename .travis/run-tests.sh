#!/bin/bash


if [ "$TRAVIS_BUILD" = "desktop" ]
then
  xvfb-run ctest -V -E 'qgis_openstreetmaptest|qgis_wcsprovidertest' -S ../qgis-test-travis.ctest --output-on-failure
else
  xvfb-run ctest -V -E 'qgis_openstreetmaptest|qgis_wcsprovidertest|PyQgsAppStartup' -S ../qgis-test-travis.ctest --output-on-failure
fi
