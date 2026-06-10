-- start query 1 in stream 0 using template query2.tpl
WITH wscs AS (
  SELECT sold_date_sk,
         sales_price
  FROM (
    SELECT ws_sold_date_sk AS sold_date_sk,
           ws_ext_sales_price AS sales_price
    FROM web_sales
    UNION ALL
    SELECT cs_sold_date_sk AS sold_date_sk,
           cs_ext_sales_price AS sales_price
    FROM catalog_sales
  ) AS sales_data
),
wswscs AS (
  SELECT d_week_seq,
         SUM(CASE WHEN d_day_name = 'Sunday' THEN sales_price ELSE '0' END) AS sun_sales,
         SUM(CASE WHEN d_day_name = 'Monday' THEN sales_price ELSE '0' END) AS mon_sales,
         SUM(CASE WHEN d_day_name = 'Tuesday' THEN sales_price ELSE '0' END) AS tue_sales,
         SUM(CASE WHEN d_day_name = 'Wednesday' THEN sales_price ELSE '0' END) AS wed_sales,
         SUM(CASE WHEN d_day_name = 'Thursday' THEN sales_price ELSE '0' END) AS thu_sales,
         SUM(CASE WHEN d_day_name = 'Friday' THEN sales_price ELSE '0' END) AS fri_sales,
         SUM(CASE WHEN d_day_name = 'Saturday' THEN sales_price ELSE '0' END) AS sat_sales
  FROM wscs,
       date_dim
  WHERE d_date_sk = sold_date_sk
  GROUP BY d_week_seq
)
SELECT d_week_seq1,
       sun_sales1 / sun_sales2 AS sun_sales_ratio,
       mon_sales1 / mon_sales2 AS mon_sales_ratio,
       tue_sales1 / tue_sales2 AS tue_sales_ratio,
       wed_sales1 / wed_sales2 AS wed_sales_ratio,
       thu_sales1 / thu_sales2 AS thu_sales_ratio,
       fri_sales1 / fri_sales2 AS fri_sales_ratio,
       sat_sales1 / sat_sales2 AS sat_sales_ratio
FROM (
  SELECT wswscs.d_week_seq AS d_week_seq1,
         sun_sales AS sun_sales1,
         mon_sales AS mon_sales1,
         tue_sales AS tue_sales1,
         wed_sales AS wed_sales1,
         thu_sales AS thu_sales1,
         fri_sales AS fri_sales1,
         sat_sales AS sat_sales1
  FROM wswscs,
       date_dim
  WHERE date_dim.d_week_seq = wswscs.d_week_seq
    AND d_year = enc_int4_encrypt('2001')
) AS y,
(
  SELECT wswscs.d_week_seq AS d_week_seq2,
         sun_sales AS sun_sales2,
         mon_sales AS mon_sales2,
         tue_sales AS tue_sales2,
         wed_sales AS wed_sales2,
         thu_sales AS thu_sales2,
         fri_sales AS fri_sales2,
         sat_sales AS sat_sales2
  FROM wswscs,
       date_dim
  WHERE date_dim.d_week_seq = wswscs.d_week_seq
    AND d_year = enc_int4_encrypt('2001') + enc_int4_encrypt('1')
) AS z
WHERE d_week_seq1 = d_week_seq2 - enc_int4_encrypt('53')
ORDER BY d_week_seq1;
-- end query 1 in stream 0 using template query2.tpl

