using ClientMinimalApi.Data.Entities;
using Microsoft.EntityFrameworkCore;

namespace ClientMinimalApi.Data
{
    public class DataContext : DbContext
    {
        public DataContext(DbContextOptions<DataContext> options) :base(options)
        {
            
        }

        protected override void OnModelCreating(ModelBuilder modelBuilder)
        {
            modelBuilder.Entity<Devices>().ToTable("Devices");
            modelBuilder.Entity<Alarm>().ToTable("AlarmData");
            modelBuilder.Entity<Card>().ToTable("Card");
            modelBuilder.Entity<Setting>().ToTable("Settings");
        }

        public DbSet<Devices> Devices { get; set; }
        public DbSet<Alarm> Alarm { get; set; }
        public DbSet<Card> Cards { get; set; }
        public DbSet<Setting> Settings { get; set; }
    }
}