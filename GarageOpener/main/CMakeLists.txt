idf_component_register(SRCS "ota.c"
                    INCLUDE_DIRS "." 
                    EMBED_TXTFILES "certs/servercert.pem" "certs/prvtkey.pem")