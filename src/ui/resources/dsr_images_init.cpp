#include <QtCore/qresource.h>

// The argument to Q_INIT_RESOURCE is the base name of the generated resource,
// which by default is the qrc file name without extension: dsr_images
static int init_dsr_images() {
    Q_INIT_RESOURCE(dsr_images);
    return 1;
}

// Run on DLL load
static int dummy_dsr_images = init_dsr_images();