-- start query 1 in stream 0 using template query14.tpl
with cross_items as (
  select i_item_sk ss_item_sk
  from item,
  (select iss.i_brand_id brand_id
      ,iss.i_class_id class_id
      ,iss.i_category_id category_id
  from store_sales
      ,item iss
      ,date_dim d1
  where ss_item_sk = iss.i_item_sk
    and ss_sold_date_sk = d1.d_date_sk
    and d1.d_year between enc_int4_encrypt('1998') AND enc_int4_encrypt('2000')
  intersect 
  select ics.i_brand_id
      ,ics.i_class_id
      ,ics.i_category_id
  from catalog_sales
      ,item ics
      ,date_dim d2
  where cs_item_sk = ics.i_item_sk
    and cs_sold_date_sk = d2.d_date_sk
    and d2.d_year between enc_int4_encrypt('1998') AND enc_int4_encrypt('2000')
  intersect
  select iws.i_brand_id
      ,iws.i_class_id
      ,iws.i_category_id
  from web_sales
      ,item iws
      ,date_dim d3
  where ws_item_sk = iws.i_item_sk
    and ws_sold_date_sk = d3.d_date_sk
    and d3.d_year between enc_int4_encrypt('1998') AND enc_int4_encrypt('2000')
  ) as intersected_items
  where i_brand_id = intersected_items.brand_id
      and i_class_id = intersected_items.class_id
      and i_category_id = intersected_items.category_id
),
avg_sales as (
  select avg(enc_int4_decrypt(quantity) * enc_float4_decrypt(list_price)) average_sales
  from (
    select ss_quantity quantity
        ,ss_list_price list_price
    from store_sales
        ,date_dim
    where ss_sold_date_sk = d_date_sk
      and d_year between enc_int4_encrypt('1998') AND enc_int4_encrypt('2000')
    union all 
    select cs_quantity quantity 
        ,cs_list_price list_price
    from catalog_sales
        ,date_dim
    where cs_sold_date_sk = d_date_sk
      and d_year between enc_int4_encrypt('1998') AND enc_int4_encrypt('2000') 
    union all
    select ws_quantity quantity
        ,ws_list_price list_price
    from web_sales
        ,date_dim
    where ws_sold_date_sk = d_date_sk
      and d_year between enc_int4_encrypt('1998') AND enc_int4_encrypt('2000')
  ) as sales_data
)
select  channel, i_brand_id, i_class_id, i_category_id, sum(sales), sum(number_sales)
from (
  select 'store' channel, i_brand_id, i_class_id, i_category_id, sum(enc_int4_decrypt(ss_quantity) * enc_float4_decrypt(ss_list_price)) sales, count(*) number_sales
  from store_sales
      ,item
      ,date_dim
  where ss_item_sk in (select ss_item_sk from cross_items)
    and ss_item_sk = i_item_sk
    and ss_sold_date_sk = d_date_sk
    and d_year = enc_int4_encrypt('2000') 
    and d_moy = enc_int4_encrypt('11')
  group by i_brand_id, i_class_id, i_category_id
  having sum(enc_int4_decrypt(ss_quantity) * enc_float4_decrypt(ss_list_price)) > (select average_sales from avg_sales)
  union all
  select 'catalog' channel, i_brand_id, i_class_id, i_category_id, sum(enc_int4_decrypt(cs_quantity) * enc_float4_decrypt(cs_list_price)) sales, count(*) number_sales
  from catalog_sales
      ,item
      ,date_dim
  where cs_item_sk in (select ss_item_sk from cross_items)
    and cs_item_sk = i_item_sk
    and cs_sold_date_sk = d_date_sk
    and d_year = enc_int4_encrypt('2000')
    and d_moy = enc_int4_encrypt('11')
  group by i_brand_id, i_class_id, i_category_id
  having sum(enc_int4_decrypt(cs_quantity) * enc_float4_decrypt(cs_list_price)) > (select average_sales from avg_sales)
  union all
  select 'web' channel, i_brand_id, i_class_id, i_category_id, sum(enc_int4_decrypt(ws_quantity) * enc_float4_decrypt(ws_list_price)) sales, count(*) number_sales
  from web_sales
      ,item
      ,date_dim
  where ws_item_sk in (select ss_item_sk from cross_items)
    and ws_item_sk = i_item_sk
    and ws_sold_date_sk = d_date_sk
    and d_year = enc_int4_encrypt('2000')
    and d_moy = enc_int4_encrypt('11')
  group by i_brand_id, i_class_id, i_category_id
  having sum(enc_int4_decrypt(ws_quantity) * enc_float4_decrypt(ws_list_price)) > (select average_sales from avg_sales)
) as combined_sales
group by rollup (channel, i_brand_id, i_class_id, i_category_id)
order by channel, i_brand_id, i_class_id, i_category_id
limit 100;

-- start second part of query
with cross_items as (
  select i_item_sk ss_item_sk
  from item,
  (select iss.i_brand_id brand_id
      ,iss.i_class_id class_id
      ,iss.i_category_id category_id
  from store_sales
      ,item iss
      ,date_dim d1
  where ss_item_sk = iss.i_item_sk
    and ss_sold_date_sk = d1.d_date_sk
    and d1.d_year between enc_int4_encrypt('1998') AND enc_int4_encrypt('2000')
  intersect
  select ics.i_brand_id
      ,ics.i_class_id
      ,ics.i_category_id
  from catalog_sales
      ,item ics
      ,date_dim d2
  where cs_item_sk = ics.i_item_sk
    and cs_sold_date_sk = d2.d_date_sk
    and d2.d_year between enc_int4_encrypt('1998') AND enc_int4_encrypt('2000')
  intersect
  select iws.i_brand_id
      ,iws.i_class_id
      ,iws.i_category_id
  from web_sales
      ,item iws
      ,date_dim d3
  where ws_item_sk = iws.i_item_sk
    and ws_sold_date_sk = d3.d_date_sk
    and d3.d_year between enc_int4_encrypt('1998') AND enc_int4_encrypt('2000')
  ) as intersected_items
  where i_brand_id = intersected_items.brand_id
      and i_class_id = intersected_items.class_id
      and i_category_id = intersected_items.category_id
),
avg_sales as (
  select avg(enc_int4_decrypt(quantity) * enc_float4_decrypt(list_price)) average_sales
  from (
    select ss_quantity quantity
        ,ss_list_price list_price
    from store_sales
        ,date_dim
    where ss_sold_date_sk = d_date_sk
      and d_year between enc_int4_encrypt('1998') AND enc_int4_encrypt('2000')
    union all
    select cs_quantity quantity
        ,cs_list_price list_price
    from catalog_sales
        ,date_dim
    where cs_sold_date_sk = d_date_sk
      and d_year between enc_int4_encrypt('1998') AND enc_int4_encrypt('2000')
    union all
    select ws_quantity quantity
        ,ws_list_price list_price
    from web_sales
        ,date_dim
    where ws_sold_date_sk = d_date_sk
      and d_year between enc_int4_encrypt('1998') AND enc_int4_encrypt('2000')
  ) as sales_data
)
select  this_year.channel ty_channel
      ,this_year.i_brand_id ty_brand
      ,this_year.i_class_id ty_class
      ,this_year.i_category_id ty_category
      ,this_year.sales ty_sales
      ,this_year.number_sales ty_number_sales
      ,last_year.channel ly_channel
      ,last_year.i_brand_id ly_brand
      ,last_year.i_class_id ly_class
      ,last_year.i_category_id ly_category
      ,last_year.sales ly_sales
      ,last_year.number_sales ly_number_sales 
from (
  select 'store' channel, i_brand_id, i_class_id, i_category_id
      ,sum(enc_int4_decrypt(ss_quantity) * enc_float4_decrypt(ss_list_price)) sales, count(*) number_sales
  from store_sales 
      ,item
      ,date_dim
  where ss_item_sk in (select ss_item_sk from cross_items)
    and ss_item_sk = i_item_sk
    and ss_sold_date_sk = d_date_sk
    and d_week_seq = (select d_week_seq
                      from date_dim
                      where d_year = enc_int4_encrypt('1999')
                        and d_moy = enc_int4_encrypt('12')
                        and d_dom = enc_int4_encrypt('16'))
  group by i_brand_id, i_class_id, i_category_id
  having sum(enc_int4_decrypt(ss_quantity) * enc_float4_decrypt(ss_list_price)) > (select average_sales from avg_sales)
) this_year,
(
  select 'store' channel, i_brand_id, i_class_id, i_category_id, sum(enc_int4_decrypt(ss_quantity) * enc_float4_decrypt(ss_list_price)) sales, count(*) number_sales
  from store_sales
      ,item
      ,date_dim
  where ss_item_sk in (select ss_item_sk from cross_items)
    and ss_item_sk = i_item_sk
    and ss_sold_date_sk = d_date_sk
    and d_week_seq = (select d_week_seq
                      from date_dim
                      where d_year = enc_int4_encrypt('1998')
                        and d_moy = enc_int4_encrypt('12')
                        and d_dom = enc_int4_encrypt('16'))
  group by i_brand_id, i_class_id, i_category_id
  having sum(enc_int4_decrypt(ss_quantity) * enc_float4_decrypt(ss_list_price)) > (select average_sales from avg_sales)
) last_year
where this_year.i_brand_id = last_year.i_brand_id
  and this_year.i_class_id = last_year.i_class_id
  and this_year.i_category_id = last_year.i_category_id
order by this_year.channel, this_year.i_brand_id, this_year.i_class_id, this_year.i_category_id
limit 100;
-- end query 1 in stream 0 using template query14.tpl
