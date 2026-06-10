-- start query 1 in stream 0 using template query44.tpl
select  i1.i_product_name best_performing, i2.i_product_name worst_performing
from(
    select *
    from (
        select item_sk
        from (
            select ss_item_sk item_sk
            from store_sales ss1
            where ss_store_sk = '2'
            group by ss_item_sk
        ) V1
    ) V11
) asceding,
(
    select *
    from (
        select item_sk
        from (
            select ss_item_sk item_sk
            from store_sales ss1
            where ss_store_sk = '2'
            group by ss_item_sk
        ) V2
    ) V21
) descending,
item i1,
item i2
where i1.i_item_sk = asceding.item_sk
  and i2.i_item_sk = descending.item_sk
limit 100;

-- end query 1 in stream 0 using template query44.tpl
