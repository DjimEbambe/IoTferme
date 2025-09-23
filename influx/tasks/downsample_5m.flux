option task = {
  name: "downsample_5m",
  cron: "0 */1 * * *",
  offset: 0m,
}

from(bucket: "iot_agg_1m")
  |> range(start: -task.every)
  |> filter(fn: (r) => r._measurement =~ /env|power|water|incubator/)
  |> aggregateWindow(every: 5m, fn: mean, createEmpty: false)
  |> to(bucket: "iot_agg_5m")
