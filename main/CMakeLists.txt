idf_component_register(SRCS "main.c"
                            "camera/app_camera.c"
                            "camera/motion.c"
                            "http_server/app_wifi.c"
                            "http_server/app_httpserver.c"
                       INCLUDE_DIRS "." "camera" "http_server" "display"
                       EMBED_FILES "html_page/index.html" 
                                   "html_page/scripts/index.js")
