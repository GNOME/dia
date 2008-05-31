enum DiaInitFlags
{
  DIA_INTERACTIVE = (1<<0),
  DIA_MESSAGE_STDERR = (1<<1)
};

void libdia_init (guint flags);

