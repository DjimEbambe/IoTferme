option task = {
  name: "downsample_1m",
  cron: "*/5 * * * *",
  offset: 0m,
}

from(bucket: "iot_raw")
  |> range(start: -task.every)
  |> filter(fn: (r) => r._measurement =~ /env|power|water|incubator/)
  |> aggregateWindow(every: 1m, fn: mean, createEmpty: false)
  |> to(bucket: "iot_agg_1m")
