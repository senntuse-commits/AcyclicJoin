-- start query 1 in stream 0 using template query70.tpl
WITH state_rankings AS (
    SELECT 
        s_state,
        RANK() OVER (ORDER BY SUM(ss_net_profit) DESC) AS ranking
    FROM 
        store_sales
        JOIN store ON s_store_sk = ss_store_sk
        JOIN date_dim ON d_date_sk = ss_sold_date_sk
    WHERE 
        d_month_seq BETWEEN 1212 AND 1212 + 11
    GROUP BY 
        s_state
),
filtered_states AS (
    SELECT 
        s_state 
    FROM 
        state_rankings
    WHERE 
        ranking <= 5
),
main_query AS (
    SELECT  
        SUM(ss_net_profit) AS total_sum,
        s_state,
        s_county,
        GROUPING(s_state) + GROUPING(s_county) AS lochierarchy,
        RANK() OVER (
            PARTITION BY GROUPING(s_state) + GROUPING(s_county), 
            CASE WHEN GROUPING(s_county) = 0 THEN s_state END
            ORDER BY SUM(ss_net_profit) DESC
        ) AS rank_within_parent
    FROM 
        store_sales
        JOIN date_dim ON d_date_sk = ss_sold_date_sk
        JOIN store ON s_store_sk = ss_store_sk
    WHERE 
        d_month_seq BETWEEN 1212 AND 1212 + 11
        AND s_state IN (SELECT s_state FROM filtered_states)
    GROUP BY 
        ROLLUP(s_state, s_county)
)
SELECT
    total_sum,
    s_state,
    s_county,
    lochierarchy,
    rank_within_parent
FROM
    main_query
ORDER BY 
    lochierarchy DESC,
    CASE WHEN lochierarchy = 0 THEN s_state END,
    rank_within_parent
LIMIT 100;
-- end query 1 in stream 0 using template query70.tpl
