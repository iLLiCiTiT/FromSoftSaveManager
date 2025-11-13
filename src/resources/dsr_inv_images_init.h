#include <QtCore/qresource.h>

// The argument to Q_INIT_RESOURCE is the base name of the generated resource,
// which by default is the qrc file name without extension: dsr_inv_images
static int init_dsr_inv_images() {
    Q_INIT_RESOURCE(dsr_inv_images);
    return 1;
}

// Run on DLL load
static int dummy_dsr_inv_images = init_dsr_inv_images();
