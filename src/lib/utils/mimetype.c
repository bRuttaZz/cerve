#include <string.h>
#include <ctype.h>

/**
@brief helper fun to transform string to lower
*/
void _convert_to_lower(char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        str[i] = tolower((unsigned char)str[i]);
    }
}

/**
@brief get mime type of a file
@param filename - filename to be used
@return mimetype string
*/
const char* get_mime_type(const char* filename) {
    char* ext = strrchr(filename, '.');
    if (!ext) {
        return "application/octet-stream"; // default
    }
    _convert_to_lower(ext);

    if (strcmp(ext, ".txt") == 0) return "text/plain";
    if (strcmp(ext, ".html") == 0) return "text/html";
    if (strcmp(ext, ".js")==0) return "text/javascript";
    if (strcmp(ext, ".css")==0) return "text/css";
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(ext, ".png") == 0) return "image/png";
    if (strcmp(ext, ".json") == 0) return "application/json";

    if (strcmp(ext, ".aac") == 0) return "audio/aac";
    if (strcmp(ext, ".abw") == 0) return "application/x-abiword";
    if (strcmp(ext, ".apng") == 0) return "image/apng";
    if (strcmp(ext, ".arc") == 0) return "application/x-freearc";
    if (strcmp(ext, ".avif") == 0) return "image/avif";
    if (strcmp(ext, ".avi") == 0) return "video/x-msvideo";
    if (strcmp(ext, ".azw") == 0) return "application/vnd.amazon.ebook";
    if (strcmp(ext, ".bin") == 0) return "application/octet-stream";
    if (strcmp(ext, ".bmp") == 0) return "image/bmp";
    if (strcmp(ext, ".bz") == 0) return "application/x-bzip";
    if (strcmp(ext, ".bz2") == 0) return "application/x-bzip2";
    if (strcmp(ext, ".cda") == 0) return "application/x-cdf";
    if (strcmp(ext, ".csh") == 0) return "application/x-csh";
    if (strcmp(ext, ".css") == 0) return "text/css";
    if (strcmp(ext, ".csv") == 0) return "text/csv";
    if (strcmp(ext, ".doc") == 0) return "application/msword";
    if (strcmp(ext, ".docx") == 0) return "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
    if (strcmp(ext, ".eot") == 0) return "application/vnd.ms-fontobject";
    if (strcmp(ext, ".epub") == 0) return "application/epub+zip";
    if (strcmp(ext, ".gz") == 0) return "application/gzip";
    if (strcmp(ext, ".gif") == 0) return "image/gif";
    if (strcmp(ext, ".htm") == 0 || strcmp(ext, ".html") == 0) return "text/html";
    if (strcmp(ext, ".ico") == 0) return "image/vnd.microsoft.icon";
    if (strcmp(ext, ".ics") == 0) return "text/calendar";
    if (strcmp(ext, ".jar") == 0) return "application/java-archive";
    if (strcmp(ext, ".jpeg") == 0 || strcmp(ext, ".jpg") == 0) return "image/jpeg";
    if (strcmp(ext, ".js") == 0) return "text/javascript";
    if (strcmp(ext, ".json") == 0) return "application/json";
    if (strcmp(ext, ".jsonld") == 0) return "application/ld+json";
    if (strcmp(ext, ".mp3") == 0) return "audio/mpeg";
    if (strcmp(ext, ".mp4") == 0) return "video/mp4";
    if (strcmp(ext, ".mpeg") == 0) return "video/mpeg";
    if (strcmp(ext, ".odt") == 0) return "application/vnd.oasis.opendocument.text";
    if (strcmp(ext, ".oga") == 0) return "audio/ogg";
    if (strcmp(ext, ".ogv") == 0) return "video/ogg";
    if (strcmp(ext, ".ogx") == 0) return "application/ogg";
    if (strcmp(ext, ".opus") == 0) return "audio/ogg";
    if (strcmp(ext, ".otf") == 0) return "font/otf";
    if (strcmp(ext, ".pdf") == 0) return "application/pdf";
    if (strcmp(ext, ".php") == 0) return "application/x-httpd-php";
    if (strcmp(ext, ".ppt") == 0) return "application/vnd.ms-powerpoint";
    if (strcmp(ext, ".pptx") == 0) return "application/vnd.openxmlformats-officedocument.presentationml.presentation";
    if (strcmp(ext, ".rar") == 0) return "application/vnd.rar";
    if (strcmp(ext, ".rtf") == 0) return "application/rtf";
    if (strcmp(ext, ".sh") == 0) return "application/x-sh";
    if (strcmp(ext, ".svg") == 0) return "image/svg+xml";
    if (strcmp(ext, ".tar") == 0) return "application/x-tar";
    if (strcmp(ext, ".tif") == 0 || strcmp(ext, ".tiff") == 0) return "image/tiff";
    if (strcmp(ext, ".ts") == 0) return "video/mp2t";
    if (strcmp(ext, ".txt") == 0) return "text/plain";
    if (strcmp(ext, ".webm") == 0) return "video/webm";
    if (strcmp(ext, ".webp") == 0) return "image/webp";
    if (strcmp(ext, ".xlsx") == 0) return "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
    if (strcmp(ext, ".xml") == 0) return "application/xml";
    if (strcmp(ext, ".zip") == 0) return "application/zip";

    return "application/octet-stream"; // default again
}
