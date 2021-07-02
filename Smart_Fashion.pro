TEMPLATE = subdirs

SUBDIRS += \
    App \
    Functional \
    LP_Plugin_HumanFeatures \
#    LP_Plugin_Import \
#    LP_Plugin_Kinectv2_Scan \
#    LP_Plugin_PIFuHD \
    LP_Plugin_Singa_Knitting \
    LP_Plugin_Singa_PC \
    Model

App.depends = Functional
Functional.depends = Model
#LP_Plugin_Singa_Knitting.depends = Functional
LP_Plugin_Singa_PC.depends = Functional
#LP_Plugin_Import.depends = Functional
#LP_Plugin_PIFuHD.depends = Functional
LP_Plugin_HumanFeature.depends = Functional
