// Copyright (C) 2020 Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
//

#include "shared_test_classes/single_layer/roi_pooling.hpp"

namespace LayerTestsDefinitions {

    std::string ROIPoolingLayerTest::getTestCaseName(testing::TestParamInfo<roiPoolingParamsTuple> obj) {
        std::vector<size_t> inputShape;
        std::vector<size_t> coordsShape;
        std::vector<size_t> poolShape;
        float spatial_scale;
        ngraph::helpers::ROIPoolingTypes pool_method;
        InferenceEngine::Precision netPrecision;
        std::string targetDevice;
        std::tie(inputShape, coordsShape, poolShape, spatial_scale, pool_method, netPrecision, targetDevice) = obj.param;

        std::ostringstream result;

        result << "IS=" << CommonTestUtils::vec2str(inputShape) << "_";
        result << "CS=" << CommonTestUtils::vec2str(coordsShape) << "_";
        result << "PS=" << CommonTestUtils::vec2str(poolShape) << "_";
        result << "Scale=" << spatial_scale << "_";
        switch (pool_method) {
            case ngraph::helpers::ROIPoolingTypes::ROI_MAX:
                result << "Max_";
                break;
            case ngraph::helpers::ROIPoolingTypes::ROI_BILINEAR:
                result << "Bilinear_";
                break;
        }
        result << "netPRC=" << netPrecision.name() << "_";
        result << "trgDev=" << targetDevice;
        return result.str();
    }

    void ROIPoolingLayerTest::Infer() {
        inferRequest = executableNetwork.CreateInferRequest();
        inputs.clear();

        auto feat_map_shape = cnnNetwork.getInputShapes().begin()->second;
        const int height = pool_method == ngraph::helpers::ROIPoolingTypes::ROI_MAX ? feat_map_shape[2] / spatial_scale : 1;
        const int width = pool_method == ngraph::helpers::ROIPoolingTypes::ROI_MAX ? feat_map_shape[3] / spatial_scale : 1;

        size_t it = 0;
        for (const auto &input : cnnNetwork.getInputsInfo()) {
            const auto &info = input.second;
            InferenceEngine::Blob::Ptr blob;

            if (it == 1) {
                blob = make_blob_with_precision(info->getTensorDesc());
                blob->allocate();
                CommonTestUtils::fill_data_roi(blob->buffer(), blob->size(), feat_map_shape[0] - 1,
                                               height, width, 1.0f);
            } else {
                blob = GenerateInput(*info);
            }
            inferRequest.SetBlob(info->name(), blob);
            inputs.push_back(blob);
            it++;
        }
        inferRequest.Infer();
    }

    void ROIPoolingLayerTest::SetUp() {
        InferenceEngine::SizeVector inputShape;
        InferenceEngine::SizeVector coordsShape;
        InferenceEngine::SizeVector poolShape;
        InferenceEngine::Precision netPrecision;
        float spatial_scale;

        std::tie(inputShape, coordsShape, poolShape, spatial_scale, pool_method, netPrecision, targetDevice) = this->GetParam();

        auto ngPrc = FuncTestUtils::PrecisionUtils::convertIE2nGraphPrc(netPrecision);
        auto params = ngraph::builder::makeParams(ngPrc, {inputShape, coordsShape});
        auto paramOuts = ngraph::helpers::convert2OutputVector(
                ngraph::helpers::castOps2Nodes<ngraph::op::Parameter>(params));
        std::shared_ptr<ngraph::Node> roi_pooling = ngraph::builder::makeROIPooling(paramOuts[0],
                                                                                    paramOuts[1],
                                                                                    poolShape,
                                                                                    spatial_scale,
                                                                                    pool_method);
        ngraph::ResultVector results{std::make_shared<ngraph::opset3::Result>(roi_pooling)};
        function = std::make_shared<ngraph::Function>(results, params, "roi_pooling");
    }
}  // namespace LayerTestsDefinitions
