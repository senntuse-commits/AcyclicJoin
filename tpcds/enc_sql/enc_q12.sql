-- start query 1 in stream 0 using template query12.tpl
SELECT
    i_item_id,
    i_item_desc,
    i_category,
    i_class,
    i_current_price,
    SUM(ws_ext_sales_price) AS itemrevenue,
    SUM(ws_ext_sales_price) * enc_float4_encrypt('100') / SUM(SUM(ws_ext_sales_price)) OVER (PARTITION BY i_class) AS revenueratio
FROM
    web_sales,
    item,
    date_dim
WHERE
    ws_item_sk = i_item_sk
    AND i_category IN ('Jewelry', 'Sports', 'Books')
    AND ws_sold_date_sk = d_date_sk
    AND d_date BETWEEN enc_timestamp_encrypt('2001-01-12')
                   AND enc_timestamp_encrypt('2001-02-11')
GROUP BY
    i_item_id,
    i_item_desc,
    i_category,
    i_class,
    i_current_price
ORDER BY
    i_category,
    i_class,
    i_item_id,
    i_item_desc,
    revenueratio
LIMIT 100;
-- end query 1 in stream 0 using template query12.tpl
