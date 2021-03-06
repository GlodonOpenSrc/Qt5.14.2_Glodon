--
-- Copyright 2019 The Android Open Source Project
--
-- Licensed under the Apache License, Version 2.0 (the "License");
-- you may not use this file except in compliance with the License.
-- You may obtain a copy of the License at
--
--     https://www.apache.org/licenses/LICENSE-2.0
--
-- Unless required by applicable law or agreed to in writing, software
-- distributed under the License is distributed on an "AS IS" BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
-- See the License for the specific language governing permissions and
-- limitations under the License.
--

SELECT RUN_METRIC('android/process_mem.sql');

CREATE VIEW malloc_memory_growth AS
SELECT
  upid,
  SUM(size) AS growth
FROM heap_profile_allocation
GROUP BY 1;

CREATE VIEW anon_and_swap_growth AS
SELECT DISTINCT
  upid,
  FIRST_VALUE(anon_and_swap_val) OVER upid_window AS start_val,
  LAST_VALUE(anon_and_swap_val) OVER upid_window AS end_val
FROM anon_and_swap_span
WINDOW upid_window AS (
  PARTITION BY upid
  ORDER BY ts
  ROWS BETWEEN UNBOUNDED PRECEDING AND UNBOUNDED FOLLOWING
);

CREATE VIEW process_growth AS
SELECT
  process.pid AS pid,
  process.name AS process_name,
  CAST(asg.start_val AS BIG INT) AS anon_and_swap_start_value,
  CAST(asg.end_val - asg.start_val AS BIG INT) AS anon_and_swap_change,
  malloc_memory_growth.growth AS malloc_change
FROM anon_and_swap_growth AS asg
JOIN process USING (upid)
LEFT JOIN malloc_memory_growth USING (upid);

CREATE VIEW instance_metrics_proto AS
SELECT AndroidProcessGrowth_InstanceMetrics(
  'pid', pid,
  'process_name', process_name,
  'anon_and_swap_start_value', anon_and_swap_start_value,
  'anon_and_swap_change_bytes', anon_and_swap_change,
  'malloc_memory_change_bytes', malloc_change
) AS instance_metric
FROM process_growth;

CREATE VIEW android_process_growth_output AS
SELECT AndroidProcessGrowth(
  'instance_metrics', (SELECT RepeatedField(instance_metric) FROM instance_metrics_proto)
);
