#pragma once

#include "l4/ClusterTypes.h"
#include "l6/ExternalSchema.h"

namespace ailee {
namespace l6 {

class FederationExport {
public:
    ExternalClusterView export_view(
        const l4::ClusterView& view
    );
};

} // namespace l6
} // namespace ailee
