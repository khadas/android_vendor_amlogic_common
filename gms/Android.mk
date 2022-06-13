# Include GMS only when the build is on the l-amai-dev branch.
# Otherwise, the build target will be duplicated, causing build
# failures.
ifeq ($(PRODUCT_USE_PREBUILT_GTVS),yes)
  include $(call all-subdir-makefiles)
endif

