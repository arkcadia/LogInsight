# Synthetic demonstration

`atlas.log` describes a fictional gateway, catalog service and worker. It contains
13 valid entries (trace 1, debug 1, info 6, warning 2, error 2, critical 1) and 3 malformed
records. The errors/critical events are log severities, not generated rule alerts.

Use **Load Sample Logs** in the desktop application, or:

```sh
./build/src/app/loginsight analyse examples/logs/atlas.log
```

All names, events and values are synthetic. No credentials or personal information are included.
