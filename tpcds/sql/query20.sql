-- start query 1 in stream 0 using template query20.tpl
SELECT i_item_id,
       i_item_desc,
       i_category,
       i_class,
       i_current_price,
       SUM(cs_ext_sales_price) AS itemrevenue,
       SUM(cs_ext_sales_price) * 100 / SUM(SUM(cs_ext_sales_price)) OVER (PARTITION BY i_class) AS revenueratio
FROM catalog_sales
JOIN item ON cs_item_sk = i_item_sk
JOIN date_dim ON cs_sold_date_sk = d_date_sk
WHERE i_category IN ('Jewelry', 'Sports', 'Books')
  AND d_date BETWEEN '2001-01-12' AND ('2001-01-12'::date + INTERVAL '30 days')
GROUP BY i_item_id,
         i_item_desc,
         i_category,
         i_class,
         i_current_price
ORDER BY i_category,
         i_class,
         i_item_id,
         i_item_desc,
         revenueratio
LIMIT 100;
-- end query 1 in stream 0 using template query20.tpl
