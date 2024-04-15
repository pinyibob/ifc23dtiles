//#define TINYGLTF_IMPLEMENTATION
//#define STB_IMAGE_IMPLEMENTATION
//#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"

#include <cstdio>
#include <fstream>
#include <iostream>

#include <nlohmann/json.hpp>

using nlohmann::json;

int XBSJ_VERTEX_SHADER = 35633;
int XBSJ_FRAGMENT_SHADER = 35632;

json getShaderJson(int shaderBufferViewID, int shaderType) {
	json jsonShader = R"({
		"bufferView": 6,
		"type": 35633,
		"extras": {
			"_pipeline": {}
		}
	})"_json;

	jsonShader["bufferView"] = shaderBufferViewID;
	jsonShader["type"] = shaderType;

	return jsonShader;
}

json getFlatJsonMaterial(int techniqueID, int textureID) {
	json jsonMaterial = R"({
		"extensions": {
			"KHR_techniques_webgl": {
				"technique": 0,
				"values" : {
					"u_diffuse": {
						"index": 0,
						"texCoord" : 0
					}
				}
			}
		},
		"emissiveFactor": [0, 0, 0],
		"alphaMode": "OPAQUE",
		"doubleSided": false
	})"_json;

	jsonMaterial["extensions"]["KHR_techniques_webgl"]["technique"] = techniqueID;
	jsonMaterial["extensions"]["KHR_techniques_webgl"]["values"]["u_diffuse"]["index"] = textureID;

	return jsonMaterial;
}

json getFlatJsonTechnique(int programid) {
	json jsonTechnique = R"({
			"attributes": {
				"a_batchid": {
					"semantic": "_BATCHID",
					"type": 5123
				},
				"a_position": {
					"semantic": "POSITION",
					"type": 35665
				},
				"a_texcoord0": {
					"semantic": "TEXCOORD_0",
					"type": 35664
				}
			},
			"program": 0,
			"states": {
				"enable": [
					2884,
					2929
				]
			},
			"uniforms": {
				"u_diffuse": {
					"type": 35678
				},
				"u_modelViewMatrix": {
					"semantic": "MODELVIEW",
					"type": 35676
				},
				"u_projectionMatrix": {
					"semantic": "PROJECTION",
					"type": 35676
				}
			}
		})"_json;

	jsonTechnique["program"] = programid;

	return jsonTechnique;
}

json getNormalJsonMaterial(int techniqueID, int textureID) {
	json jsonMaterial = R"({
      "extensions": {
        "KHR_techniques_webgl": {
          "values": {
            "u_texImage": {
              "index": 0,
              "texCoord": 0
            },
            "u_diffuse": [
              1,
              1,
              1,
              1
            ],
            "u_ambient": [
              0.5882353186607361,
              0.5882353186607361,
              0.5882353186607361,
              1
            ],
            "u_specular": [
              0.8999999761581421,
              0.8999999761581421,
              0.8999999761581421,
              1
            ],
            "u_emissive": [
              0,
              0,
              0,
              1
            ],
            "u_shinniness": 0,
            "u_light": 1
          },
          "technique": 0
        }
      },
      "emissiveFactor": [
        0,
        0,
        0
      ],
      "alphaMode": "OPAQUE",
      "doubleSided": false
	})"_json;

	jsonMaterial["extensions"]["KHR_techniques_webgl"]["technique"] = techniqueID;
	jsonMaterial["extensions"]["KHR_techniques_webgl"]["values"]["u_texImage"]["index"] = textureID;

	return jsonMaterial;
}

json getNormalJsonTechnique(int programid) {
	json jsonTechnique = R"({
          "attributes": {
            "a_batchId": {
              "semantic": "_BATCHID",
              "type": 5123
            },
            "a_normal": {
              "semantic": "NORMAL",
              "type": 35665
            },
            "a_position": {
              "semantic": "POSITION",
              "type": 35665
            },
            "a_texcoord0": {
              "semantic": "TEXCOORD_0",
              "type": 35664
            }
          },
          "uniforms": {
            "u_ambient": {
              "type": 35666
            },
            "u_diffuse": {
              "type": 35666
            },
            "u_light": {
              "type": 5126
            },
            "u_modelViewMatrix": {
              "semantic": "MODELVIEW",
              "type": 35676
            },
            "u_normalMatrix": {
              "semantic": "MODELVIEWINVERSETRANSPOSE",
              "type": 35675
            },
            "u_projectionMatrix": {
              "semantic": "PROJECTION",
              "type": 35676
            },
            "u_shininess": {
              "type": 5126
            },
            "u_specular": {
              "type": 35666
            },
            "u_texImage": {
              "type": 35678
            }
          },
          "program": 0,
          "states": {
            "enable": [
              2884,
              2929
            ]
          }
		})"_json;

	jsonTechnique["program"] = programid;

	return jsonTechnique;
}


