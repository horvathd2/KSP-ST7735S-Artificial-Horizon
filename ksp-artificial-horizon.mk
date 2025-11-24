################################################################################
# ksp-artificial-horizon Buildroot package
################################################################################

KSP_ARTIFICIAL_HORIZON_VERSION = 1.0
KSP_ARTIFICIAL_HORIZON_SITE = /home/hdani/buildroot/package/ksp-artificial-horizon
KSP_ARTIFICIAL_HORIZON_SITE_METHOD = local

KSP_ARTIFICIAL_HORIZON_DEPENDENCIES = libgpiod

define KSP_ARTIFICIAL_HORIZON_BUILD_CMDS
	$(MAKE) CC="$(TARGET_CC)" -C $(@D)
endef

define KSP_ARTIFICIAL_HORIZON_INSTALL_INIT_SYSV
    $(INSTALL) -m 0755 -D $(KSP_ARTIFICIAL_HORIZON_PKGDIR)/S99kspah \
        $(TARGET_DIR)/etc/init.d/S99kspah
endef

define KSP_ARTIFICIAL_HORIZON_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/lcd_app \
		$(TARGET_DIR)/usr/bin/lcd_app
endef

$(eval $(generic-package))
