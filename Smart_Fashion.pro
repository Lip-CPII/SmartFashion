TEMPLATE = subdirs

SUBDIRS += \
    App \
    Functional \
    LP_Plugin_HumanFeatures \
    LP_Plugin_Import \
    LP_Plugin_PIFuHD \
    Model

App.depends = Functional
Functional.depends = Model
LP_Plugin_Import.depends = Functional
LP_Plugin_PIFuHD.depends = Functional
LP_Plugin_HumanFeature.depends = Functional
