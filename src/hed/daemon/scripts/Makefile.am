if SYSV_SCRIPTS_ENABLED
HED_SCRIPT = arched
else
HED_SCRIPT =
endif
initd_SCRIPTS = $(HED_SCRIPT)

if SYSTEMD_UNITS_ENABLED
HED_UNIT = arched.service
HED_UNIT_WRAPPER = arched-start
else
HED_UNIT =
HED_UNIT_WRAPPER =
endif
units_DATA = $(HED_UNIT)
pkgdata_SCRIPTS = $(HED_UNIT_WRAPPER)
