idf_component_register(SRCS "http_client.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES esp_event esp_http_client
                    EMBED_TXTFILES certificate.pem)