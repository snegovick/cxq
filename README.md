# CXQ - the xq replacement

XQ is an awesome tool for xml querrying in bash scripts, but it is huge (~7 MB) which is not very nice for some embedded systems.

CXQ is based on libxml2 xpath example, and is tiny itself. Combined with libxml2 the size of CXQ is about half of the size of XQ (depending on configuration of libxml2), most of which is occupied by the library.

## Examples

Lookup apn nodes from apns db:

```
cxq -f apns-full-conf.xml -x /apns/apn[@mcc="260"][@mnc="06"]
```

Print values of attribute "user" from apns db:

```
cxq -f apns-full-conf.xml -x /apns/apn[@mcc="260"][@mnc="06"]/@user
```
