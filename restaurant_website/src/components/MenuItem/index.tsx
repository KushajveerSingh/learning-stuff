import styles from './MenuItem.module.css';

interface Props {
  title: string;
  price: string;
  tags: string;
}

const MenuItem = ({ title, price, tags }: Props) => {
  return (
    <div className={styles.app__menuitem}>
      <div className={styles.app__menuitem_head}>
        <div className={styles.app__menuitem_name}>
          <p className="p__cormorant" style={{ color: '#DCCA87' }}>
            {title}
          </p>
        </div>

        <div className={styles.app__menuitem_dash} />

        <div className={styles.app__menuitem_price}>
          <p className="p__cormorant">{price}</p>
        </div>
      </div>

      <div className={styles.app__menuitem_sub}>
        <p className="p__opensans" style={{ color: '#AAAAAA' }}>
          {tags}
        </p>
      </div>
    </div>
  );
};

export default MenuItem;
