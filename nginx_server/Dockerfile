# Use the official Nginx image based on Alpine Linux
FROM nginx:alpine

# Copy our custom HTML page to Nginx's default serving directory
COPY data/. /usr/share/nginx/html

COPY nginx.conf /etc/nginx/conf.d/default.conf
