-- start query 1 in stream 0 using template query36.tpl
SELECT
    SUM(ss_net_profit) / SUM(ss_ext_sales_price) AS gross_margin,
    i_category,
    i_class,
    grouping(i_category) + grouping(i_class) AS lochierarchy,
    RANK() OVER (
        PARTITION BY grouping(i_category) + grouping(i_class),
                     CASE WHEN grouping(i_class) = '0' THEN i_category END
        ORDER BY SUM(ss_net_profit) / SUM(ss_ext_sales_price) ASC
    ) AS rank_within_parent
FROM
    store_sales
    JOIN date_dim d1 ON d1.d_date_sk = ss_sold_date_sk
    JOIN item ON i_item_sk = ss_item_sk
    JOIN store ON s_store_sk = ss_store_sk
WHERE
    d1.d_year = '2000'
    AND s_state IN ('TN', 'TN', 'TN', 'TN', 'TN', 'TN', 'TN', 'TN')
GROUP BY ROLLUP(i_category, i_class)
ORDER BY
    grouping(i_category) + grouping(i_class) DESC,
    CASE WHEN grouping(i_category) = 0 THEN i_category END,
    rank_within_parent
LIMIT 100;

-- end query 1 in stream 0 using template query36.tpl
